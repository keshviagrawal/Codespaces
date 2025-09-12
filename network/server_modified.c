#define _POSIX_C_SOURCE 200112L
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <openssl/md5.h>

#include "sham_modified.h"

#define RECV_BUFFER_CAP (SHAM_DATA_SIZE * 64) // Internal reassembly buffer capacity
#define OOO_BUFFER_SLOTS 128                  // Number of slots for out-of-order packets

// Represents an out-of-order data chunk buffered by the receiver
struct ooo_chunk {
    bool used;
    uint32_t seq;
    size_t len;
    uint8_t data[SHAM_DATA_SIZE];
};

// Holds all state for a single connection
struct connection {
    sham_state state;
    struct sockaddr_in peer_addr;
    uint32_t rcv_nxt; // Next expected sequence number from client
    uint32_t snd_nxt; // Next sequence number to send
    // File transfer state
    int out_fd;
    bool got_filename;
    MD5_CTX md5_ctx;
    // Reassembly buffer state
    size_t reassembly_used_bytes;
    struct ooo_chunk ooo_buf[OOO_BUFFER_SLOTS];
};

static void die(const char *msg) {
    perror(msg);
    exit(1);
}

// --- Packet Transmission/Reception ---

static ssize_t send_sham_packet(int sock, const struct sockaddr_in *peer, uint32_t seq, uint32_t ack, uint16_t flags, uint16_t win, const uint8_t *data, size_t len) {
    uint8_t buf[sizeof(struct sham_header) + SHAM_DATA_SIZE];
    struct sham_header h;
    h.seq_num = htonl(seq);
    h.ack_num = htonl(ack);
    h.flags = htons(flags);
    h.window_size = htons(win);
    memcpy(buf, &h, sizeof(h));
    if (len > 0) memcpy(buf + sizeof(h), data, len);
    return sendto(sock, buf, sizeof(h) + len, 0, (const struct sockaddr *)peer, sizeof(*peer));
}

static int recv_sham_packet(int sock, struct sham_header *h, uint8_t *data, size_t *len, struct sockaddr_in *src) {
    uint8_t buf[sizeof(struct sham_header) + SHAM_DATA_SIZE];
    socklen_t sl = sizeof(*src);
    ssize_t n = recvfrom(sock, buf, sizeof(buf), 0, (struct sockaddr *)src, &sl);
    if (n < 0) return -1;
    if ((size_t)n < sizeof(struct sham_header)) return -2; // Packet too small
    memcpy(h, buf, sizeof(*h));
    h->seq_num = ntohl(h->seq_num);
    h->ack_num = ntohl(h->ack_num);
    h->flags = ntohs(h->flags);
    h->window_size = ntohs(h->window_size);
    *len = (size_t)n - sizeof(struct sham_header);
    if (*len > 0 && data) memcpy(data, buf + sizeof(struct sham_header), *len);
    return 0;
}

// --- State Management & Helpers ---

static uint16_t calc_window(const struct connection *conn) {
    size_t free_bytes = (RECV_BUFFER_CAP > conn->reassembly_used_bytes) ? (RECV_BUFFER_CAP - conn->reassembly_used_bytes) : 0;
    return (free_bytes > 0xFFFF) ? 0xFFFF : (uint16_t)free_bytes;
}

static void process_ooo_buffer(struct connection *conn, bool chat_mode) {
    bool progressed = true;
    while (progressed) {
        progressed = false;
        for (int i = 0; i < OOO_BUFFER_SLOTS; ++i) {
            if (conn->ooo_buf[i].used && conn->ooo_buf[i].seq == conn->rcv_nxt) {
                uint8_t *data = conn->ooo_buf[i].data;
                size_t len = conn->ooo_buf[i].len;

                if (chat_mode) {
                    write(STDOUT_FILENO, data, len);
                } else {
                    if (write(conn->out_fd, data, len) != (ssize_t)len) die("write from ooo_buffer");
                    MD5_Update(&conn->md5_ctx, data, len);
                }
                conn->rcv_nxt += len;
                conn->ooo_buf[i].used = false;
                progressed = true;
            }
        }
    }
}

static void handle_data_packet(int sock, struct connection *conn, const struct sham_header *h, const uint8_t *data, size_t len, bool chat_mode, FILE *logf) {
    rudp_logf(logf, "RCV DATA SEQ=%u LEN=%zu", h->seq_num, len);

    // --- In-order data ---
    if (h->seq_num == conn->rcv_nxt) {
        // Special first message in file mode: FNAME:<name>\n
        if (!chat_mode && !conn->got_filename) {
            if (len > strlen(FILENAME_BANNER) && strncmp((const char*)data, FILENAME_BANNER, strlen(FILENAME_BANNER)) == 0) {
                char outname[256];
                const char *name_start = (const char*)data + strlen(FILENAME_BANNER);
                size_t name_len = len - strlen(FILENAME_BANNER);
                if (name_len >= sizeof(outname)) name_len = sizeof(outname) - 1;
                memcpy(outname, name_start, name_len);
                char *nl = memchr(outname, '\n', name_len); // Trim newline
                if (nl) *nl = '\0';

                conn->out_fd = open(outname, O_CREAT | O_TRUNC | O_WRONLY, 0644);
                if (conn->out_fd < 0) die("open output file");
                conn->got_filename = true;
            }
        } else { // Regular data
            if (chat_mode) {
                write(STDOUT_FILENO, data, len);
            } else {
                if (write(conn->out_fd, data, len) != (ssize_t)len) die("write");
                MD5_Update(&conn->md5_ctx, data, len);
            }
        }
        conn->rcv_nxt += len;
        process_ooo_buffer(conn, chat_mode); // Check if this unlocks buffered packets
    }
    // --- Out-of-order data ---
    else if (h->seq_num > conn->rcv_nxt) {
        // Buffer if slot available
        for (int i = 0; i < OOO_BUFFER_SLOTS; ++i) {
            if (!conn->ooo_buf[i].used) {
                conn->ooo_buf[i].used = true;
                conn->ooo_buf[i].seq = h->seq_num;
                conn->ooo_buf[i].len = len;
                memcpy(conn->ooo_buf[i].data, data, len);
                break;
            }
        }
    }
    // --- Duplicate/old segment ---
    // else: Do nothing, just send cumulative ACK below.

    // Always send a cumulative ACK for any data packet received
    uint16_t win = calc_window(conn);
    send_sham_packet(sock, &conn->peer_addr, conn->snd_nxt, conn->rcv_nxt, SHAM_ACK, win, NULL, 0);
    rudp_logf(logf, "SND ACK=%u WIN=%u", conn->rcv_nxt, win);
    rudp_logf(logf, "FLOW WIN UPDATE=%u", win);
}

// --- Main State Machine Logic ---

void handle_incoming_packet(int sock, struct connection *conn, bool chat_mode, double loss_rate, FILE *logf) {
    struct sham_header h;
    uint8_t data[SHAM_DATA_SIZE];
    size_t dlen = 0;
    struct sockaddr_in src_addr;

    if (recv_sham_packet(sock, &h, data, &dlen, &src_addr) < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) return; // No packet
        die("recvfrom");
    }

    // State-based processing
    switch (conn->state) {
        case S_LISTEN:
            if (h.flags == SHAM_SYN) {
                conn->peer_addr = src_addr;
                conn->state = S_SYN_RCVD;
                conn->rcv_nxt = h->seq_num + 1;
                conn->snd_nxt = (uint32_t)(rand() ^ (uint32_t)sham_now_ms());
                rudp_logf(logf, "RCV SYN SEQ=%u", h.seq_num);

                uint16_t win = calc_window(conn);
                send_sham_packet(sock, &conn->peer_addr, conn->snd_nxt, conn->rcv_nxt, SHAM_SYN | SHAM_ACK, win, NULL, 0);
                rudp_logf(logf, "SND SYN-ACK SEQ=%u ACK=%u", conn->snd_nxt, conn->rcv_nxt);
                conn->snd_nxt++;
            }
            break;

        case S_SYN_RCVD:
            if (h.flags == SHAM_ACK && h.ack_num == conn->snd_nxt) {
                conn->state = S_ESTABLISHED;
                rudp_logf(logf, "RCV ACK FOR SYN");
                fprintf(stderr, "Server: Connection established.\n");
            }
            break;

        case S_ESTABLISHED:
            // Enforce same peer
            if (src_addr.sin_port != conn->peer_addr.sin_port || src_addr.sin_addr.s_addr != conn->peer_addr.sin_addr.s_addr) {
                break;
            }
            // Handle FIN from client
            if (h.flags & SHAM_FIN) {
                rudp_logf(logf, "RCV FIN SEQ=%u", h.seq_num);
                conn->rcv_nxt = h.seq_num + 1;
                uint16_t win = calc_window(conn);
                send_sham_packet(sock, &conn->peer_addr, conn->snd_nxt, conn->rcv_nxt, SHAM_ACK, win, NULL, 0);
                rudp_logf(logf, "SND ACK FOR FIN");

                conn->state = S_LAST_ACK; // Transition to LAST_ACK
                send_sham_packet(sock, &conn->peer_addr, conn->snd_nxt, conn->rcv_nxt, SHAM_FIN, 0, NULL, 0);
                rudp_logf(logf, "SND FIN SEQ=%u", conn->snd_nxt);
                conn->snd_nxt++;
                break;
            }
            // Handle DATA
            if (dlen > 0) {
                if (rudp_should_drop(loss_rate)) {
                    rudp_logf(logf, "DROP DATA SEQ=%u", h.seq_num);
                } else {
                    handle_data_packet(sock, conn, &h, data, dlen, chat_mode, logf);
                }
            }
            break;

        case S_LAST_ACK:
            if (h.flags == SHAM_ACK && h.ack_num == conn->snd_nxt) {
                conn->state = S_CLOSED;
                fprintf(stderr, "Server: Connection closed.\n");
            }
            break;

        default:
            // Ignore packets in other states for this simplified server
            break;
    }
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <port> [--chat] [loss_rate]\n", argv[0]);
        return 1;
    }
    int argi = 1;
    int port = atoi(argv[argi++]);
    bool chat_mode = false;
    double loss_rate = 0.0;
    if (argi < argc && strcmp(argv[argi], "--chat") == 0) { chat_mode = true; argi++; }
    if (argi < argc) { loss_rate = atof(argv[argi++]); }

    srand((unsigned)time(NULL) ^ (unsigned)getpid());
    FILE* logf = rudp_log_open("server_log.txt");

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) die("socket");

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons((uint16_t)port);
    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) die("bind");

    struct connection conn = {0};
    conn.state = S_LISTEN;
    MD5_Init(&conn.md5_ctx);

    fprintf(stderr, "Server listening on port %d%s (loss=%.3f)\n", port, chat_mode ? " [chat]" : "", loss_rate);

    while (conn.state != S_CLOSED) {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(sock, &rfds);
        if (chat_mode && conn.state == S_ESTABLISHED) FD_SET(STDIN_FILENO, &rfds);

        struct timeval tv = { .tv_sec = 1, .tv_usec = 0 };
        int rv = select(sock + 1, &rfds, NULL, NULL, &tv);
        if (rv < 0) {
            if (errno == EINTR) continue;
            die("select");
        }

        if (FD_ISSET(sock, &rfds)) {
            handle_incoming_packet(sock, &conn, chat_mode, loss_rate, logf);
        }

        if (chat_mode && conn.state == S_ESTABLISHED && FD_ISSET(STDIN_FILENO, &rfds)) {
            char line[SHAM_DATA_SIZE];
            ssize_t n = read(STDIN_FILENO, line, sizeof(line));
            if (n > 0) {
                // For simplicity, server chat send is best-effort, no retransmissions
                send_sham_packet(sock, &conn.peer_addr, conn.snd_nxt, conn.rcv_nxt, 0, calc_window(&conn), (uint8_t*)line, n);
                rudp_logf(logf, "SND DATA SEQ=%u LEN=%zd", conn.snd_nxt, n);
                conn.snd_nxt += n;
            }
        }
    }

    if (!chat_mode && conn.out_fd >= 0) {
        unsigned char md5res[MD5_DIGEST_LENGTH];
        MD5_Final(md5res, &conn.md5_ctx);
        printf("MD5: ");
        for (int i = 0; i < MD5_DIGEST_LENGTH; ++i) {
            printf("%02x", md5res[i]);
        }
        printf("\n");
        close(conn.out_fd);
    }

    if (logf) fclose(logf);
    close(sock);
    return 0;
}
