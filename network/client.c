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
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "sham.h"

struct txpkt {
    bool in_use;
    uint32_t seq;          // first byte seq
    size_t len;            // data length
    uint8_t data[SHAM_DATA_SIZE];
    uint64_t last_tx_ms;   // last send time
};

static void die(const char *msg) {
    perror(msg);
    exit(1);
}

static ssize_t send_sham(int sock, const struct sockaddr_in *peer,
                         uint32_t seq, uint32_t ack, uint16_t flags, uint16_t win,
                         const uint8_t *data, size_t len)
{
    uint8_t buf[sizeof(struct sham_header) + SHAM_DATA_SIZE];
    struct sham_header h;
    h.seq_num = htonl(seq);
    h.ack_num = htonl(ack);
    h.flags = htons(flags);
    h.window_size = htons(win);
    memcpy(buf, &h, sizeof(h));
    if (len > 0) memcpy(buf + sizeof(h), data, len);
    return sendto(sock, buf, sizeof(h) + len, 0,
                  (const struct sockaddr *)peer, sizeof(*peer));
}

static int recv_packet(int sock, struct sham_header *h, uint8_t *data, size_t *len)
{
    uint8_t buf[sizeof(struct sham_header) + SHAM_DATA_SIZE];
    ssize_t n = recv(sock, buf, sizeof(buf), 0);
    if (n < 0) return -1;
    if ((size_t)n < sizeof(struct sham_header)) return -2;
    memcpy(h, buf, sizeof(*h));
    h->seq_num = ntohl(h->seq_num);
    h->ack_num = ntohl(h->ack_num);
    h->flags = ntohs(h->flags);
    h->window_size = ntohs(h->window_size);
    *len = (size_t)n - sizeof(struct sham_header);
    if (*len > 0 && data) memcpy(data, buf + sizeof(struct sham_header), *len);
    return 0;
}

static void add_or_update_tx(struct txpkt window[], size_t slots, uint32_t seq, const uint8_t* data, size_t len) {
    for (size_t i = 0; i < slots; ++i) {
        if (!window[i].in_use) {
            window[i].in_use = true;
            window[i].seq = seq;
            window[i].len = len;
            memcpy(window[i].data, data, len);
            window[i].last_tx_ms = sham_now_ms();
            return;
        }
    }
}

int main(int argc, char **argv) {
    // CLI:
    // File mode: ./client <server_ip> <server_port> <input_file> <output_file_name> [loss_rate]
    // Chat mode: ./client <server_ip> <server_port> --chat [loss_rate]
    if (argc < 4) {
        fprintf(stderr, "Usage:\n  %s <server_ip> <server_port> <input_file> <output_file_name> [loss_rate]\n  %s <server_ip> <server_port> --chat [loss_rate]\n", argv[0], argv[0]);
        return 1;
    }

    const char *ip = argv[1];
    int port = atoi(argv[2]);
    bool chat_mode = false;
    const char *infile = NULL;
    const char *outname = NULL;
    double loss_rate = 0.0;

    int argi = 3;
    if (strcmp(argv[argi], "--chat") == 0) {
        chat_mode = true;
        argi++;
    } else {
        if (argc < 5) {
            fprintf(stderr, "Usage: %s <server_ip> <server_port> <input_file> <output_file_name> [loss_rate]\n", argv[0]);
            return 1;
        }
        infile = argv[argi++];
        outname = argv[argi++];
    }
    if (argi < argc) {
        loss_rate = atof(argv[argi++]);
        if (loss_rate < 0.0) loss_rate = 0.0;
        if (loss_rate > 1.0) loss_rate = 1.0;
    }

    srand((unsigned)time(NULL) ^ (unsigned)getpid());
    FILE* logf = rudp_log_open("client_log.txt");

    int infd = -1;
    struct stat st;
    if (!chat_mode) {
        infd = open(infile, O_RDONLY);
        if (infd < 0) die("open input");
        if (fstat(infd, &st) < 0) die("fstat");
    }

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) die("socket");

    struct sockaddr_in serv = {0};
    serv.sin_family = AF_INET;
    serv.sin_port = htons((uint16_t)port);
    if (inet_pton(AF_INET, ip, &serv.sin_addr) != 1) die("inet_pton");

    // Connection variables
    uint32_t iss = (uint32_t)(rand() ^ (uint32_t)sham_now_ms());
    uint32_t snd_nxt = iss + 1; // after SYN (SYN consumes one)
    uint32_t irs = 0;           // server initial seq
    uint32_t rcv_nxt = 0;       // next expected from server
    uint16_t rcv_adv_window = 65535; // server advertised window (bytes)

    // Sliding window and tx buffer
    struct txpkt window[SHAM_SENDER_WINDOW_PKTS] = {0};
    size_t inflight_bytes = 0;

    // Three-way handshake
    if (send_sham(sock, &serv, iss, 0, SHAM_SYN, 0, NULL, 0) < 0) die("send SYN");
    rudp_logf(logf, "SND SYN SEQ=%u", iss);
    fprintf(stderr, "Client: sent SYN iss=%u\n", iss);

    while (1) {
        fd_set rfds; FD_ZERO(&rfds); FD_SET(sock, &rfds);
        struct timeval tv = { .tv_sec = 1, .tv_usec = 0 };
        int rv = select(sock + 1, &rfds, NULL, NULL, &tv);
        if (rv < 0) {
            if (errno == EINTR) continue;
            die("select");
        }
        if (!FD_ISSET(sock, &rfds)) continue;

        struct sham_header h; uint8_t dummy[1]; size_t dlen = 0;
        if (recv_packet(sock, &h, dummy, &dlen) == 0) {
            if ((h.flags & (SHAM_SYN | SHAM_ACK)) == (SHAM_SYN | SHAM_ACK) && h.ack_num == snd_nxt) {
                irs = h.seq_num;
                rcv_nxt = irs + 1;
                rcv_adv_window = h.window_size;
                if (send_sham(sock, &serv, snd_nxt, rcv_nxt, SHAM_ACK, 0, NULL, 0) < 0) die("send ACK");
                rudp_logf(logf, "RCV SYN SEQ=%u", irs); // handshake visibility
                rudp_logf(logf, "RCV ACK FOR SYN");      // after we sent ACK
                fprintf(stderr, "Client: handshake complete irs=%u\n", irs);
                break;
            }
        }
    }

    if (chat_mode) {
        // Chat loop: stdin + socket, basic cumulative ACK on receive
        bool closing = false;
        for (;;) {
            fd_set rfds; FD_ZERO(&rfds); FD_SET(sock, &rfds); FD_SET(0, &rfds);
            struct timeval tv = { .tv_sec = 0, .tv_usec = 200000 };
            int rv = select(sock + 1, &rfds, NULL, NULL, &tv);
            if (rv < 0) {
                if (errno == EINTR) continue;
                die("select chat");
            }

            if (FD_ISSET(0, &rfds) && !closing) {
                char line[SHAM_DATA_SIZE];
                ssize_t n = read(0, line, sizeof(line));
                if (n > 0) {
                    if (n >= 6 && !memcmp(line, "/quit\n", 6)) {
                        // Initiate FIN
                        uint32_t fin_seq = snd_nxt;
                        snd_nxt += 1;
                        send_sham(sock, &serv, fin_seq, rcv_nxt, SHAM_FIN, 0, NULL, 0);
                        rudp_logf(logf, "SND FIN SEQ=%u", fin_seq);
                        closing = true;
                    } else {
                        // Respect receiver window (roughly)
                        if (inflight_bytes + (size_t)n <= rcv_adv_window) {
                            add_or_update_tx(window, SHAM_SENDER_WINDOW_PKTS, snd_nxt, (uint8_t*)line, (size_t)n);
                            if (send_sham(sock, &serv, snd_nxt, rcv_nxt, 0, 0, (uint8_t*)line, (size_t)n) < 0) die("send chat");
                            rudp_logf(logf, "SND DATA SEQ=%u LEN=%zd", snd_nxt, (ssize_t)n);
                            inflight_bytes += (size_t)n;
                            snd_nxt += (uint32_t)n;
                        }
                    }
                }
            }

            if (FD_ISSET(sock, &rfds)) {
                struct sham_header h; uint8_t data[SHAM_DATA_SIZE]; size_t dlen = 0;
                if (recv_packet(sock, &h, data, &dlen) == 0) {
                    // Receiver-side packet loss for DATA only
                    if (dlen > 0 && !(h.flags & (SHAM_SYN | SHAM_FIN))) {
                        if (rudp_should_drop(loss_rate)) {
                            rudp_logf(logf, "DROP DATA SEQ=%u");
                            // Send current ACK for cumulative state
                            send_sham(sock, &serv, snd_nxt, rcv_nxt, SHAM_ACK, 0, NULL, 0);
                            rudp_logf(logf, "SND ACK=%u WIN=%u", rcv_nxt, 0u);
                            continue;
                        }
                    }

                    if (dlen > 0) {
                        // Display to stdout only if in-order
                        if (h.seq_num == rcv_nxt) {
                            write(1, data, dlen);
                            rcv_nxt += (uint32_t)dlen;
                        }
                        // ACK any time we receive data
                        send_sham(sock, &serv, snd_nxt, rcv_nxt, SHAM_ACK, 0, NULL, 0);
                        rudp_logf(logf, "RCV DATA SEQ=%u LEN=%zu", h.seq_num, dlen);
                        rudp_logf(logf, "SND ACK=%u WIN=%u", rcv_nxt, 0u);
                    }

                    if (h.flags & SHAM_ACK) {
                        rudp_logf(logf, "RCV ACK=%u", h.ack_num);
                        // Free fully-acked sends
                        for (size_t i = 0; i < SHAM_SENDER_WINDOW_PKTS; ++i) {
                            struct txpkt *p = &window[i];
                            if (!p->in_use) continue;
                            uint32_t end = p->seq + (uint32_t)p->len;
                            if (end <= h.ack_num) {
                                if (inflight_bytes >= p->len) inflight_bytes -= p->len;
                                p->in_use = false;
                            }
                        }
                        // Update adv window
                        rcv_adv_window = h.window_size;
                        rudp_logf(logf, "FLOW WIN UPDATE=%u", (unsigned)rcv_adv_window);
                    }

                    if (h.flags & SHAM_FIN) {
                        // ACK their FIN and if we’re not closing, send our FIN too
                        rcv_nxt = h.seq_num + 1;
                        send_sham(sock, &serv, snd_nxt, rcv_nxt, SHAM_ACK, 0, NULL, 0);
                        rudp_logf(logf, "RCV FIN SEQ=%u", h.seq_num);
                        rudp_logf(logf, "SND ACK FOR FIN");
                        if (!closing) {
                            uint32_t fin_seq = snd_nxt;
                            snd_nxt += 1;
                            send_sham(sock, &serv, fin_seq, rcv_nxt, SHAM_FIN, 0, NULL, 0);
                            rudp_logf(logf, "SND FIN SEQ=%u", fin_seq);
                            closing = true;
                        } else {
                            // Already sent our FIN; wait for ACK to our FIN (implicit by ack_num)
                            // When we receive ACK for our FIN number, can exit
                        }
                    }
                }
            }

            // Retransmit timeouts (chat)
            uint64_t now = sham_now_ms();
            for (size_t i = 0; i < SHAM_SENDER_WINDOW_PKTS; ++i) {
                struct txpkt *p = &window[i];
                if (!p->in_use) continue;
                if (now - p->last_tx_ms >= SHAM_RTO_MS) {
                    if (send_sham(sock, &serv, p->seq, rcv_nxt, 0, 0, p->data, p->len) < 0) die("re-send chat data");
                    rudp_logf(logf, "TIMEOUT SEQ=%u", p->seq);
                    rudp_logf(logf, "RETX DATA SEQ=%u LEN=%zu", p->seq, p->len);
                    p->last_tx_ms = now;
                }
            }

            // Exit condition: heuristic — if we already initiated FIN and receive ACK that covers our FIN
            // Not strictly necessary; typically the peer FIN will drive closure
            // For simplicity, break on closing + idle timeout
            if (closing) {
                // allow a short grace period for final ACKs
                static uint64_t close_start = 0;
                if (!close_start) close_start = sham_now_ms();
                if (sham_now_ms() - close_start > 1000) break;
            }
        }

        if (logf) fclose(logf);
        close(sock);
        return 0;
    }

    // -------------- File transfer mode --------------

    // 1) Send filename banner as first data message
    char fname_msg[SHAM_DATA_SIZE];
    int fname_len = snprintf(fname_msg, sizeof(fname_msg), "FNAME:%s\n", outname);
    if (fname_len < 0 || fname_len > (int)sizeof(fname_msg)) die("fname snprintf");
    add_or_update_tx(window, SHAM_SENDER_WINDOW_PKTS, snd_nxt, (uint8_t*)fname_msg, (size_t)fname_len);
    if (send_sham(sock, &serv, snd_nxt, rcv_nxt, 0, 0, (uint8_t*)fname_msg, (size_t)fname_len) < 0) die("send fname");
    rudp_logf(logf, "SND DATA SEQ=%u LEN=%d", snd_nxt, fname_len);
    inflight_bytes += (size_t)fname_len;
    snd_nxt += (uint32_t)fname_len;

    // 2) Stream the file using sliding window
    bool eof = false;
    for (;;) {
        // Fill window subject to slots and receiver window
        while (!eof) {
            size_t inflight_pkts = 0;
            for (size_t i = 0; i < SHAM_SENDER_WINDOW_PKTS; ++i)
                if (window[i].in_use) inflight_pkts++;

            if (inflight_pkts >= SHAM_SENDER_WINDOW_PKTS) break;
            if (inflight_bytes >= rcv_adv_window) break;

            uint8_t buf[SHAM_DATA_SIZE];
            ssize_t n = read(infd, buf, SHAM_DATA_SIZE);
            if (n < 0) die("read");
            if (n == 0) { eof = true; break; }

            add_or_update_tx(window, SHAM_SENDER_WINDOW_PKTS, snd_nxt, buf, (size_t)n);
            if (send_sham(sock, &serv, snd_nxt, rcv_nxt, 0, 0, buf, (size_t)n) < 0) die("send data");
            rudp_logf(logf, "SND DATA SEQ=%u LEN=%zd", snd_nxt, (ssize_t)n);
            snd_nxt += (uint32_t)n;
            inflight_bytes += (size_t)n;
        }

        // Termination when fully acked
        if (eof && inflight_bytes == 0) {
            uint32_t fin_seq = snd_nxt;
            snd_nxt += 1;
            if (send_sham(sock, &serv, fin_seq, rcv_nxt, SHAM_FIN, 0, NULL, 0) < 0) die("send FIN");
            rudp_logf(logf, "SND FIN SEQ=%u", fin_seq);

            // Wait for ACK to our FIN and server FIN, then final ACK
            bool fin_acked = false, fin_rcvd = false;
            uint64_t fin_tx = sham_now_ms();

            while (!(fin_acked && fin_rcvd)) {
                fd_set rfds; FD_ZERO(&rfds); FD_SET(sock, &rfds);
                struct timeval tv = { .tv_sec = 0, .tv_usec = 200000 };
                int rv = select(sock + 1, &rfds, NULL, NULL, &tv);
                if (rv < 0) { if (errno == EINTR) continue; die("select during FIN"); }
                if (FD_ISSET(sock, &rfds)) {
                    struct sham_header h; uint8_t d[SHAM_DATA_SIZE]; size_t dlen = 0;
                    if (recv_packet(sock, &h, d, &dlen) == 0) {
                        if ((h.flags & SHAM_ACK) && h.ack_num == snd_nxt) {
                            rudp_logf(logf, "RCV ACK=%u", h.ack_num);
                            fin_acked = true;
                        }
                        if (h.flags & SHAM_FIN) {
                            rcv_nxt = h.seq_num + 1;
                            send_sham(sock, &serv, snd_nxt, rcv_nxt, SHAM_ACK, 0, NULL, 0);
                            rudp_logf(logf, "RCV FIN SEQ=%u", h.seq_num);
                            rudp_logf(logf, "SND ACK FOR FIN");
                            fin_rcvd = true;
                        }
                        rcv_adv_window = h.window_size;
                        rudp_logf(logf, "FLOW WIN UPDATE=%u", (unsigned)rcv_adv_window);
                    }
                }
                if (!fin_acked && sham_now_ms() - fin_tx >= SHAM_RTO_MS) {
                    if (send_sham(sock, &serv, fin_seq, rcv_nxt, SHAM_FIN, 0, NULL, 0) < 0) die("re-send FIN");
                    rudp_logf(logf, "TIMEOUT SEQ=%u", fin_seq);
                    fin_tx = sham_now_ms();
                }
            }
            break;
        }

        // Wait for ACKs or timeouts
        fd_set rfds; FD_ZERO(&rfds); FD_SET(sock, &rfds);
        struct timeval tv = { .tv_sec = 0, .tv_usec = 100000 };
        int rv = select(sock + 1, &rfds, NULL, NULL, &tv);
        if (rv < 0) { if (errno == EINTR) continue; die("select loop"); }

        if (FD_ISSET(sock, &rfds)) {
            struct sham_header h; uint8_t d[SHAM_DATA_SIZE]; size_t dlen = 0;
            if (recv_packet(sock, &h, d, &dlen) == 0) {
                // Simulate loss only for data we RECEIVE (rare in file mode)
                if (dlen > 0 && !(h.flags & (SHAM_SYN | SHAM_FIN))) {
                    if (rudp_should_drop(loss_rate)) {
                        rudp_logf(logf, "DROP DATA SEQ=%u", h.seq_num);
                        send_sham(sock, &serv, snd_nxt, rcv_nxt, SHAM_ACK, 0, NULL, 0);
                        rudp_logf(logf, "SND ACK=%u WIN=%u", rcv_nxt, 0u);
                        continue;
                    }
                }

                if (h.flags & SHAM_ACK) {
                    rudp_logf(logf, "RCV ACK=%u", h.ack_num);
                    uint32_t ack = h.ack_num;
                    for (size_t i = 0; i < SHAM_SENDER_WINDOW_PKTS; ++i) {
                        struct txpkt *p = &window[i];
                        if (!p->in_use) continue;
                        uint32_t end = p->seq + (uint32_t)p->len;
                        if (end <= ack) {
                            if (inflight_bytes >= p->len) inflight_bytes -= p->len;
                            p->in_use = false;
                        }
                    }
                    rcv_adv_window = h.window_size;
                    rudp_logf(logf, "FLOW WIN UPDATE=%u", (unsigned)rcv_adv_window);
                }

                if (h.flags & SHAM_FIN) {
                    rcv_nxt = h.seq_num + 1;
                    send_sham(sock, &serv, snd_nxt, rcv_nxt, SHAM_ACK, 0, NULL, 0);
                    rudp_logf(logf, "RCV FIN SEQ=%u", h.seq_num);
                    rudp_logf(logf, "SND ACK FOR FIN");
                    // If we haven't sent FIN, do it now (race)
                    if (!eof) {
                        uint32_t fin_seq = snd_nxt;
                        snd_nxt += 1;
                        if (send_sham(sock, &serv, fin_seq, rcv_nxt, SHAM_FIN, 0, NULL, 0) < 0) die("send FIN after server");
                        rudp_logf(logf, "SND FIN SEQ=%u", fin_seq);
                        eof = true;
                    }
                }
            }
        }

        // Timeout retransmissions
        uint64_t now = sham_now_ms();
        for (size_t i = 0; i < SHAM_SENDER_WINDOW_PKTS; ++i) {
            struct txpkt *p = &window[i];
            if (!p->in_use) continue;
            if (now - p->last_tx_ms >= SHAM_RTO_MS) {
                if (send_sham(sock, &serv, p->seq, rcv_nxt, 0, 0, p->data, p->len) < 0) die("re-send data");
                rudp_logf(logf, "TIMEOUT SEQ=%u", p->seq);
                rudp_logf(logf, "RETX DATA SEQ=%u LEN=%zu", p->seq, p->len);
                p->last_tx_ms = now;
            }
        }
    }

    if (!chat_mode && infd >= 0) close(infd);
    if (logf) fclose(logf);
    close(sock);
    return 0;
}
