// // server.c

// #include "sham.h"
// #include <unistd.h>
// #include <sys/socket.h>
// #include <netinet/in.h>
// #include <fcntl.h>
// #include <sys/time.h>

// void pack_sham_packet(char* buffer, const sham_header* header, const char* payload, size_t payload_len) {
//     uint32_t net_seq_num = htonl(header->seq_num);
//     uint32_t net_ack_num = htonl(header->ack_num);
//     uint16_t net_flags = htons(header->flags);
//     uint16_t net_window = htons(header->window_size);

//     memcpy(buffer, &net_seq_num, sizeof(net_seq_num));
//     memcpy(buffer + sizeof(net_seq_num), &net_ack_num, sizeof(net_ack_num));
//     memcpy(buffer + sizeof(net_seq_num) + sizeof(net_ack_num), &net_flags, sizeof(net_flags));
//     memcpy(buffer + sizeof(net_seq_num) + sizeof(net_ack_num) + sizeof(net_flags), &net_window, sizeof(net_window));
//     if (payload_len > 0) {
//         memcpy(buffer + sizeof(sham_header), payload, payload_len);
//     }
// }

// void unpack_sham_packet(const char* buffer, sham_header* header, char* payload, size_t* payload_len) {
//     uint32_t net_seq_num, net_ack_num;
//     uint16_t net_flags, net_window;

//     memcpy(&net_seq_num, buffer, sizeof(net_seq_num));
//     memcpy(&net_ack_num, buffer + sizeof(net_seq_num), sizeof(net_ack_num));
//     memcpy(&net_flags, buffer + sizeof(net_seq_num) + sizeof(net_ack_num), sizeof(net_flags));
//     memcpy(&net_window, buffer + sizeof(net_seq_num) + sizeof(net_ack_num) + sizeof(net_flags), sizeof(net_window));

//     header->seq_num = ntohl(net_seq_num);
//     header->ack_num = ntohl(net_ack_num);
//     header->flags = ntohs(net_flags);
//     header->window_size = ntohs(net_window);

//     if (payload && payload_len) {
//         *payload_len = 0;
//     }
// }

// long get_time_diff(struct timeval start, struct timeval end) {
//     return (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
// }

// // Global variables for simplicity
// int sockfd;
// struct sockaddr_in cliaddr;
// socklen_t len = sizeof(cliaddr);
// char rcv_buffer[BUFFER_SIZE + sizeof(sham_header)];

// // --- Connection Management ---
// int three_way_handshake() {
//     printf("SERVER: Waiting for three-way handshake...\n");

//     // Step 1: Client -> Server (SYN)
//     ssize_t n = recvfrom(sockfd, rcv_buffer, sizeof(rcv_buffer), 0, (struct sockaddr*)&cliaddr, &len);
//     if (n < (ssize_t)sizeof(sham_header)) {
//         printf("SERVER: Invalid packet size. Handshake failed.\n");
//         return -1;
//     }
//     sham_header syn_header;
//     unpack_sham_packet(rcv_buffer, &syn_header, NULL, NULL);
//     if (!(syn_header.flags & SYN)) {
//         printf("SERVER: Invalid SYN packet received. Handshake failed.\n");
//         return -1;
//     }
//     printf("SERVER: Received SYN from client. Client SEQ=%u\n", syn_header.seq_num);
    
//     // Step 2: Server -> Client (SYN-ACK)
//     sham_header syn_ack_header = { .seq_num = 67890, .ack_num = syn_header.seq_num + 1, .flags = SYN | ACK, .window_size = 0 };
//     char packet[sizeof(sham_header)];
//     pack_sham_packet(packet, &syn_ack_header, NULL, 0);
//     sendto(sockfd, packet, sizeof(sham_header), 0, (struct sockaddr*)&cliaddr, len);
//     printf("SERVER: Sent SYN-ACK with SEQ=%u, ACK=%u\n", syn_ack_header.seq_num, syn_ack_header.ack_num);

//     // Step 3: Client -> Server (ACK)
//     struct timeval timeout = {1, 0};
//     setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
//     n = recvfrom(sockfd, rcv_buffer, sizeof(rcv_buffer), 0, (struct sockaddr*)&cliaddr, &len);
//     if (n <= 0) {
//         printf("SERVER: Timeout waiting for final ACK. Handshake failed.\n");
//         return -1;
//     }
//     sham_header ack_header;
//     unpack_sham_packet(rcv_buffer, &ack_header, NULL, NULL);
//     if (!(ack_header.flags & ACK) || ack_header.ack_num != syn_ack_header.seq_num + 1) {
//         printf("SERVER: Invalid final ACK received. Handshake failed.\n");
//         return -1;
//     }
//     printf("SERVER: Received final ACK. Connection established.\n");
//     return 0;
// }

// void four_way_handshake() {
//     printf("SERVER: Initiating four-way handshake...\n");
//     // Step 1: Client -> Server (FIN)
//     // The main loop will break on receiving FIN.

//     // Step 2: Server -> Client (ACK)
//     sham_header ack_header = { .flags = ACK };
//     char packet[sizeof(sham_header)];
//     pack_sham_packet(packet, &ack_header, NULL, 0);
//     sendto(sockfd, packet, sizeof(sham_header), 0, (struct sockaddr*)&cliaddr, len);
//     printf("SERVER: Sent ACK for client's FIN.\n");

//     // Step 3: Server -> Client (FIN)
//     sham_header fin_header = { .flags = FIN };
//     pack_sham_packet(packet, &fin_header, NULL, 0);
//     sendto(sockfd, packet, sizeof(sham_header), 0, (struct sockaddr*)&cliaddr, len);
//     printf("SERVER: Sent own FIN.\n");

//     // Step 4: Client -> Server (ACK)
//     struct timeval timeout = {1, 0};
//     setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
//     ssize_t n = recvfrom(sockfd, rcv_buffer, sizeof(rcv_buffer), 0, (struct sockaddr*)&cliaddr, &len);
//     if (n <= 0) {
//         printf("SERVER: Timeout or error waiting for final ACK. Closing socket.\n");
//     } else {
//         sham_header final_ack;
//         unpack_sham_packet(rcv_buffer, &final_ack, NULL, NULL);
//         if (final_ack.flags & ACK) {
//             printf("SERVER: Received final ACK. Connection closed.\n");
//         }
//     }
// }

// // --- Main Logic ---
// int main(int argc, char *argv[]) {
//     if (argc != 2) {
//         fprintf(stderr, "Usage: %s <port>\n", argv[0]);
//         exit(1);
//     }
//     int port = atoi(argv[1]);

//     sockfd = socket(AF_INET, SOCK_DGRAM, 0);
//     if (sockfd < 0) {
//         perror("socket creation failed");
//         exit(EXIT_FAILURE);
//     }

//     memset(&cliaddr, 0, sizeof(cliaddr));
//     struct sockaddr_in servaddr;
//     memset(&servaddr, 0, sizeof(servaddr));
//     servaddr.sin_family = AF_INET;
//     servaddr.sin_addr.s_addr = INADDR_ANY;
//     servaddr.sin_port = htons(port);
    
//     if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
//         perror("bind failed");
//         exit(EXIT_FAILURE);
//     }

//     if (three_way_handshake() != 0) {
//         close(sockfd);
//         return 1;
//     }
    
//     // Simulating file reception logic
//     printf("SERVER: Starting file reception. Waiting for data...\n");
//     char payload_buffer[BUFFER_SIZE];
//     uint32_t expected_seq = 12346;
    
//     while(1) {
//         ssize_t n = recvfrom(sockfd, rcv_buffer, sizeof(rcv_buffer), 0, (struct sockaddr*)&cliaddr, &len);
//         if (n <= 0) {
//             // No more packets, assume transfer is over
//             break; 
//         }

//         sham_header rcv_header;
//         unpack_sham_packet(rcv_buffer, &rcv_header, payload_buffer, NULL);

//         if (rcv_header.flags & FIN) {
//             four_way_handshake();
//             break;
//         }

//         if (rcv_header.seq_num == expected_seq) {
//             // In-order packet received, update expected seq
//             printf("SERVER: RCV DATA SEQ=%u. Sending cumulative ACK=%u\n", rcv_header.seq_num, rcv_header.seq_num + (uint32_t)(n - sizeof(sham_header)));
            
//             // This is a simplified cumulative ACK; a real impl would track payload length
//             sham_header ack_header = { .seq_num = 0, .ack_num = rcv_header.seq_num + (n - sizeof(sham_header)), .flags = ACK, .window_size = 0 };
//             char packet[sizeof(sham_header)];
//             pack_sham_packet(packet, &ack_header, NULL, 0);
//             sendto(sockfd, packet, sizeof(sham_header), 0, (struct sockaddr*)&cliaddr, len);
//             expected_seq += (n - sizeof(sham_header));
            
//         } else if (rcv_header.seq_num > expected_seq) {
//             // Out-of-order packet
//             printf("SERVER: RCV DATA SEQ=%u (out-of-order). Sending ACK for next expected SEQ=%u\n", rcv_header.seq_num, expected_seq);
//             sham_header ack_header = { .seq_num = 0, .ack_num = expected_seq, .flags = ACK, .window_size = 0 };
//             char packet[sizeof(sham_header)];
//             pack_sham_packet(packet, &ack_header, NULL, 0);
//             sendto(sockfd, packet, sizeof(sham_header), 0, (struct sockaddr*)&cliaddr, len);
//         } else {
//             // Duplicate packet
//             printf("SERVER: Duplicate packet SEQ=%u. Ignoring.\n", rcv_header.seq_num);
//         }
//     }
    
//     close(sockfd);
//     return 0;
// }




// #define _POSIX_C_SOURCE 200112L
// #include <arpa/inet.h>
// #include <errno.h>
// #include <fcntl.h>
// #include <netinet/in.h>
// #include <stdbool.h>
// #include <stdint.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <sys/select.h>
// #include <sys/socket.h>
// #include <sys/types.h>
// #include <unistd.h>

// #include "sham.h"

// #define RECV_BUFFER_CAP (SHAM_DATA_SIZE * 64) // internal reassembly buffer capacity

// struct ooo_chunk {
//     uint32_t seq; // first byte seq
//     size_t len;
//     uint8_t data[SHAM_DATA_SIZE];
//     bool used;
// };

// static void die(const char *msg) {
//     perror(msg);
//     exit(1);
// }

// static ssize_t send_sham(int sock, const struct sockaddr_in *peer,
//                          uint32_t seq, uint32_t ack, uint16_t flags, uint16_t win,
//                          const uint8_t *data, size_t len)
// {
//     uint8_t buf[sizeof(struct sham_header) + SHAM_DATA_SIZE];
//     struct sham_header h;
//     h.seq_num = htonl(seq);
//     h.ack_num = htonl(ack);
//     h.flags = htons(flags);
//     h.window_size = htons(win);
//     memcpy(buf, &h, sizeof(h));
//     if (len > 0) memcpy(buf + sizeof(h), data, len);
//     return sendto(sock, buf, sizeof(h) + len, 0,
//                   (const struct sockaddr *)peer, sizeof(*peer));
// }

// static int recv_packet(int sock, struct sham_header *h, uint8_t *data, size_t *len,
//                        struct sockaddr_in *src)
// {
//     uint8_t buf[sizeof(struct sham_header) + SHAM_DATA_SIZE];
//     socklen_t sl = sizeof(*src);
//     ssize_t n = recvfrom(sock, buf, sizeof(buf), 0, (struct sockaddr *)src, &sl);
//     if (n < 0) return -1;
//     if ((size_t)n < sizeof(struct sham_header)) return -2;
//     memcpy(h, buf, sizeof(*h));
//     h->seq_num = ntohl(h->seq_num);
//     h->ack_num = ntohl(h->ack_num);
//     h->flags = ntohs(h->flags);
//     h->window_size = ntohs(h->window_size);
//     *len = (size_t)n - sizeof(struct sham_header);
//     if (*len > 0 && data) memcpy(data, buf + sizeof(struct sham_header), *len);
//     return 0;
// }

// // Calculate advertised window based on free reassembly capacity
// static uint16_t calc_window(size_t reassembly_used) {
//     size_t freeb = (RECV_BUFFER_CAP > reassembly_used) ? (RECV_BUFFER_CAP - reassembly_used) : 0;
//     if (freeb > 0xFFFF) freeb = 0xFFFF;
//     return (uint16_t)freeb;
// }

// int main(int argc, char **argv) {
//     if (argc != 3) {
//         fprintf(stderr, "Usage: %s <port> <output_file>\n", argv[0]);
//         return 1;
//     }

//     int port = atoi(argv[1]);
//     const char *outfile = argv[2];

//     int outfd = open(outfile, O_CREAT | O_TRUNC | O_WRONLY, 0644);
//     if (outfd < 0) die("open output");

//     int sock = socket(AF_INET, SOCK_DGRAM, 0);
//     if (sock < 0) die("socket");

//     struct sockaddr_in addr = {0};
//     addr.sin_family = AF_INET;
//     addr.sin_addr.s_addr = htonl(INADDR_ANY);
//     addr.sin_port = htons((uint16_t)port);
//     if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) die("bind");

//     // Connection state
//     struct sockaddr_in client = {0};
//     bool connected = false;
//     uint32_t iss = (uint32_t) (rand() ^ (uint32_t)sham_now_ms());
//     uint32_t rcv_nxt = 0; // next expected byte from client
//     uint32_t snd_una = 0; // oldest unacked seq we sent (for control pkts)
//     uint32_t snd_nxt = 0; // next seq we will use for control (SYN/FIN)

//     // Reassembly accounting and OOO buffer
//     size_t reassembly_used = 0; // contiguous written bytes since start of stream
//     struct ooo_chunk ooo[128] = {0};

//     fprintf(stderr, "Server listening on port %d\n", port);

//     // Main loop
//     while (1) {
//         fd_set rfds; FD_ZERO(&rfds); FD_SET(sock, &rfds);
//         struct timeval tv = { .tv_sec = 1, .tv_usec = 0 };
//         int rv = select(sock + 1, &rfds, NULL, NULL, &tv);
//         if (rv < 0) {
//             if (errno == EINTR) continue;
//             die("select");
//         }

//         if (!FD_ISSET(sock, &rfds)) continue;

//         struct sham_header h; uint8_t data[SHAM_DATA_SIZE]; size_t dlen = 0;
//         struct sockaddr_in src;
//         int r = recv_packet(sock, &h, data, &dlen, &src);
//         if (r < 0) continue;

//         // New client or same
//         if (!connected) {
//             if ((h.flags & SHAM_SYN) && !(h.flags & SHAM_ACK)) {
//                 client = src;
//                 uint32_t irs = h.seq_num;
//                 rcv_nxt = irs + 1; // SYN consumes one
//                 iss = (uint32_t)(rand() ^ (uint32_t)sham_now_ms());
//                 snd_una = iss;
//                 snd_nxt = iss + 1; // SYN consumes one
//                 uint16_t win = calc_window(reassembly_used);
//                 send_sham(sock, &client, iss, rcv_nxt, SHAM_SYN | SHAM_ACK, win, NULL, 0);
//                 connected = true;
//                 fprintf(stderr, "SYN received, sent SYN-ACK iss=%u ack=%u\n", iss, rcv_nxt);
//             }
//             continue;
//         }

//         // Enforce from same client
//         if (src.sin_port != client.sin_port || src.sin_addr.s_addr != client.sin_addr.s_addr) {
//             // Ignore stray
//             continue;
//         }

//         // Final handshake ACK
//         if ((h.flags & SHAM_ACK) && !(h.flags & SHAM_SYN) && snd_una == iss && h.ack_num == snd_nxt) {
//             snd_una = snd_nxt; // our SYN acknowledged
//             fprintf(stderr, "Handshake completed\n");
//             continue;
//         }

//         // FIN handling
//         if (h.flags & SHAM_FIN) {
//             rcv_nxt = h.seq_num + 1;
//             // ACK their FIN
//             send_sham(sock, &client, snd_nxt, rcv_nxt, SHAM_ACK, calc_window(reassembly_used), NULL, 0);
//             // Send our FIN
//             uint32_t fin_seq = snd_nxt;
//             snd_nxt += 1;
//             send_sham(sock, &client, fin_seq, rcv_nxt, SHAM_FIN, calc_window(reassembly_used), NULL, 0);
//             fprintf(stderr, "FIN received and sent FIN\n");
//             break;
//         }

//         // Data
//         if (dlen > 0) {
//             // If in-order
//             if (h.seq_num == rcv_nxt) {
//                 // Write contiguous
//                 if (reassembly_used + dlen <= RECV_BUFFER_CAP) {
//                     ssize_t wr = write(outfd, data, dlen);
//                     if (wr != (ssize_t)dlen) die("write");
//                     reassembly_used += dlen;
//                     rcv_nxt += (uint32_t)dlen;

//                     // Check buffered OOO to advance
//                     bool progressed = true;
//                     while (progressed) {
//                         progressed = false;
//                         for (size_t i = 0; i < sizeof(ooo)/sizeof(ooo[0]); ++i) {
//                             if (ooo[i].used && ooo[i].seq == rcv_nxt) {
//                                 if (reassembly_used + ooo[i].len <= RECV_BUFFER_CAP) {
//                                     ssize_t wr2 = write(outfd, ooo[i].data, ooo[i].len);
//                                     if (wr2 != (ssize_t)ooo[i].len) die("write");
//                                     reassembly_used += ooo[i].len;
//                                     rcv_nxt += (uint32_t)ooo[i].len;
//                                     ooo[i].used = false;
//                                     progressed = true;
//                                 }
//                             }
//                         }
//                     }
//                 }
//             } else if (h.seq_num > rcv_nxt) {
//                 // Out-of-order, buffer if slot available
//                 for (size_t i = 0; i < sizeof(ooo)/sizeof(ooo[0]); ++i) {
//                     if (!ooo[i].used) {
//                         ooo[i].used = true;
//                         ooo[i].seq = h.seq_num;
//                         ooo[i].len = dlen;
//                         memcpy(ooo[i].data, data, dlen);
//                         break;
//                     }
//                 }
//             } else {
//                 // Duplicate/old segment, ignore
//             }

//             // Send cumulative ACK with current advertised window
//             uint16_t win = calc_window(reassembly_used);
//             send_sham(sock, &client, snd_nxt, rcv_nxt, SHAM_ACK, win, NULL, 0);
//         } else {
//             // Pure ACK handling: nothing to do for server data path (server doesn't send data)
//             (void)h;
//         }
//     }

//     close(outfd);
//     close(sock);
//     fprintf(stderr, "Connection closed.\n");
//     return 0;
// }


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

#include "sham.h"

#define RECV_BUFFER_CAP (SHAM_DATA_SIZE * 64) // internal reassembly buffer capacity

struct ooo_chunk {
    uint32_t seq; // first byte seq
    size_t len;
    uint8_t data[SHAM_DATA_SIZE];
    bool used;
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

static int recv_packet(int sock, struct sham_header *h, uint8_t *data, size_t *len,
                       struct sockaddr_in *src)
{
    uint8_t buf[sizeof(struct sham_header) + SHAM_DATA_SIZE];
    socklen_t sl = sizeof(*src);
    ssize_t n = recvfrom(sock, buf, sizeof(buf), 0, (struct sockaddr *)src, &sl);
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

// Calculate advertised window based on free reassembly capacity
static uint16_t calc_window(size_t reassembly_used) {
    size_t freeb = (RECV_BUFFER_CAP > reassembly_used) ? (RECV_BUFFER_CAP - reassembly_used) : 0;
    if (freeb > 0xFFFF) freeb = 0xFFFF;
    return (uint16_t)freeb;
}

static bool starts_with(const uint8_t* buf, size_t len, const char* prefix) {
    size_t plen = strlen(prefix);
    return len >= plen && memcmp(buf, prefix, plen) == 0;
}

int main(int argc, char **argv) {
    // CLI: ./server <port> [--chat] [loss_rate]
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <port> [--chat] [loss_rate]\n", argv[0]);
        return 1;
    }
    int argi = 1;
    int port = atoi(argv[argi++]);
    bool chat_mode = false;
    double loss_rate = 0.0;
    if (argi < argc && strcmp(argv[argi], "--chat") == 0) { chat_mode = true; argi++; }
    if (argi < argc) { loss_rate = atof(argv[argi++]); if (loss_rate < 0.0) loss_rate = 0.0; if (loss_rate > 1.0) loss_rate = 1.0; }

    srand((unsigned)time(NULL) ^ (unsigned)getpid());

    FILE* logf = rudp_log_open("server_log.txt");

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) die("socket");

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons((uint16_t)port);
    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) die("bind");

    struct sockaddr_in client = {0};
    bool connected = false;
    uint32_t iss = (uint32_t) (rand() ^ (uint32_t)sham_now_ms());
    uint32_t rcv_nxt = 0; // next expected byte from client
    uint32_t snd_una = 0;
    uint32_t snd_nxt = 0;

    // File transfer state
    int outfd = -1;
    char outname[256] = {0};
    bool got_filename = false;
    MD5_CTX md5;
    MD5_Init(&md5);

    // Reassembly and OOO
    size_t reassembly_used = 0;
    struct ooo_chunk ooo[128] = {0};

    fprintf(stderr, "Server listening on port %d%s (loss=%.3f)\n", port, chat_mode ? " [chat]" : "", loss_rate);

    // Handshake + main loop
    for (;;) {
        fd_set rfds; FD_ZERO(&rfds); FD_SET(sock, &rfds);
        if (chat_mode && connected) FD_SET(0, &rfds); // stdin for chat
        struct timeval tv = { .tv_sec = 1, .tv_usec = 0 };
        int rv = select(sock + 1, &rfds, NULL, NULL, &tv);
        if (rv < 0) {
            if (errno == EINTR) continue;
            die("select");
        }

        // Chat stdin send path (server sending data to client in chat)
        if (chat_mode && connected && FD_ISSET(0, &rfds)) {
            char line[SHAM_DATA_SIZE];
            ssize_t n = read(0, line, sizeof(line));
            if (n <= 0) {
                // Treat as no input; ignore
            } else {
                if (n >= 6 && !memcmp(line, "/quit\n", 6)) {
                    // Initiate FIN
                    uint32_t fin_seq = snd_nxt;
                    snd_nxt += 1;
                    send_sham(sock, &client, fin_seq, rcv_nxt, SHAM_FIN, calc_window(reassembly_used), NULL, 0);
                    rudp_logf(logf, "SND FIN SEQ=%u", fin_seq);
                    // We’ll let the normal FIN handling finalize below upon responses
                } else {
                    // Send as a single DATA
                    send_sham(sock, &client, snd_nxt, rcv_nxt, 0, calc_window(reassembly_used), (uint8_t*)line, (size_t)n);
                    rudp_logf(logf, "SND DATA SEQ=%u LEN=%zd", snd_nxt, (ssize_t)n);
                    snd_nxt += (uint32_t)n;
                }
            }
        }

        if (!FD_ISSET(sock, &rfds)) continue;

        struct sham_header h; uint8_t data[SHAM_DATA_SIZE]; size_t dlen = 0;
        struct sockaddr_in src;
        // uint8_t rawbuf[sizeof(struct sham_header) + SHAM_DATA_SIZE]; // not needed, kept consistent
        int r = recv_packet(sock, &h, data, &dlen, &src);
        if (r < 0) continue;

        if (!connected) {
            if ((h.flags & SHAM_SYN) && !(h.flags & SHAM_ACK)) {
                client = src;
                rudp_logf(logf, "RCV SYN SEQ=%u", h.seq_num);
                uint32_t irs = h.seq_num;
                rcv_nxt = irs + 1; // SYN consumes one
                iss = (uint32_t)(rand() ^ (uint32_t)sham_now_ms());
                snd_una = iss;
                snd_nxt = iss + 1; // SYN consumes one
                uint16_t win = calc_window(reassembly_used);
                send_sham(sock, &client, iss, rcv_nxt, SHAM_SYN | SHAM_ACK, win, NULL, 0);
                rudp_logf(logf, "SND SYN-ACK SEQ=%u ACK=%u", iss, rcv_nxt);
                connected = true;
                continue;
            } else {
                continue;
            }
        }

        // Enforce same peer
        if (src.sin_port != client.sin_port || src.sin_addr.s_addr != client.sin_addr.s_addr) {
            continue;
        }

        // Final handshake ACK
        if ((h.flags & SHAM_ACK) && !(h.flags & SHAM_SYN) && snd_una == iss && h.ack_num == snd_nxt) {
            snd_una = snd_nxt; // our SYN acknowledged
            rudp_logf(logf, "RCV ACK FOR SYN");
            continue;
        }

        // Simulated packet loss: only drop DATA (payload > 0 and not SYN/FIN)
        if (dlen > 0 && !(h.flags & (SHAM_SYN | SHAM_FIN))) {
            if (rudp_should_drop(loss_rate)) {
                rudp_logf(logf, "DROP DATA SEQ=%u", h.seq_num);
                // Still respond with current ACK (cumulative), to mimic not having received this segment
                uint16_t win = calc_window(reassembly_used);
                send_sham(sock, &client, snd_nxt, rcv_nxt, SHAM_ACK, win, NULL, 0);
                rudp_logf(logf, "SND ACK=%u WIN=%u", rcv_nxt, (unsigned)win);
                continue;
            }
        }

        // FIN handling
        if (h.flags & SHAM_FIN) {
            rudp_logf(logf, "RCV FIN SEQ=%u", h.seq_num);
            rcv_nxt = h.seq_num + 1;
            // ACK their FIN
            uint16_t win = calc_window(reassembly_used);
            send_sham(sock, &client, snd_nxt, rcv_nxt, SHAM_ACK, win, NULL, 0);
            rudp_logf(logf, "SND ACK FOR FIN");
            // Send our FIN
            uint32_t fin_seq = snd_nxt;
            snd_nxt += 1;
            send_sham(sock, &client, fin_seq, rcv_nxt, SHAM_FIN, calc_window(reassembly_used), NULL, 0);
            rudp_logf(logf, "SND FIN SEQ=%u", fin_seq);
            break;
        }

        // ACKs from client (chat mode could see these due to our sends)
        if ((h.flags & SHAM_ACK) && dlen == 0 && !(h.flags & SHAM_SYN)) {
            rudp_logf(logf, "RCV ACK=%u", h.ack_num);
            // No server data window bookkeeping here (server uses best-effort send in chat)
        }

        // Data path
        if (dlen > 0) {
            rudp_logf(logf, "RCV DATA SEQ=%u LEN=%zu", h.seq_num, dlen);

            // Special first message in file mode: FNAME:<name>\n
            if (!chat_mode && !got_filename && h.seq_num == rcv_nxt && starts_with(data, dlen, "FNAME:")) {
                const char *prefix = "FNAME:";
                size_t plen = strlen(prefix);
                size_t nlen = dlen - plen;
                if (nlen >= sizeof(outname)) nlen = sizeof(outname)-1;
                memcpy(outname, data + plen, nlen);
                // trim newline if present
                char *nl = memchr(outname, '\n', nlen);
                if (nl) *nl = '\0';
                // Open output
                outfd = open(outname, O_CREAT | O_TRUNC | O_WRONLY, 0644);
                if (outfd < 0) die("open output");
                got_filename = true;

                // advance rcv_nxt
                rcv_nxt += (uint32_t)dlen;

                // ACK after consuming filename
                uint16_t win = calc_window(reassembly_used);
                send_sham(sock, &client, snd_nxt, rcv_nxt, SHAM_ACK, win, NULL, 0);
                rudp_logf(logf, "SND ACK=%u WIN=%u", rcv_nxt, (unsigned)win);
                continue;
            }

            // In-order data
            if (h.seq_num == rcv_nxt) {
                if (chat_mode) {
                    // Print to stdout
                    write(1, data, dlen);
                } else {
                    if (!got_filename) {
                        // If filename not received in order, we can’t write file data yet; ignore content but ACK rcv_nxt (still expecting filename)
                    } else {
                        // write to file and update MD5
                        if (reassembly_used + dlen <= RECV_BUFFER_CAP) {
                            if (write(outfd, data, dlen) != (ssize_t)dlen) die("write");
                            MD5_Update(&md5, data, dlen);
                            reassembly_used += dlen;
                            rcv_nxt += (uint32_t)dlen;
                            // drain OOO
                            bool progressed = true;
                            while (progressed) {
                                progressed = false;
                                for (size_t i = 0; i < sizeof(ooo)/sizeof(ooo[0]); ++i) {
                                    if (ooo[i].used && ooo[i].seq == rcv_nxt) {
                                        if (reassembly_used + ooo[i].len <= RECV_BUFFER_CAP) {
                                            if (write(outfd, ooo[i].data, ooo[i].len) != (ssize_t)ooo[i].len) die("write");
                                            MD5_Update(&md5, ooo[i].data, ooo[i].len);
                                            reassembly_used += ooo[i].len;
                                            rcv_nxt += (uint32_t)ooo[i].len;
                                            ooo[i].used = false;
                                            progressed = true;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            } else if (h.seq_num > rcv_nxt) {
                // Out-of-order, buffer if slot available; in chat we don’t buffer, but we can still stash to keep ACK logic consistent
                for (size_t i = 0; i < sizeof(ooo)/sizeof(ooo[0]); ++i) {
                    if (!ooo[i].used) {
                        ooo[i].used = true;
                        ooo[i].seq = h.seq_num;
                        ooo[i].len = dlen;
                        memcpy(ooo[i].data, data, dlen);
                        break;
                    }
                }
            } else {
                // Duplicate/old segment
            }

            // Send cumulative ACK with current advertised window
            uint16_t win = calc_window(reassembly_used);
            send_sham(sock, &client, snd_nxt, rcv_nxt, SHAM_ACK, win, NULL, 0);
            rudp_logf(logf, "SND ACK=%u WIN=%u", rcv_nxt, (unsigned)win);
            rudp_logf(logf, "FLOW WIN UPDATE=%u", (unsigned)win);
        }
    }

    if (!chat_mode) {
        // Print MD5 to stdout on success close (file transfer mode)
        if (outfd >= 0) {
            unsigned char md5res[MD5_DIGEST_LENGTH];
            MD5_Final(md5res, &md5);
            char hex[MD5_DIGEST_LENGTH*2 + 1];
            for (int i = 0; i < MD5_DIGEST_LENGTH; ++i)
                sprintf(&hex[i*2], "%02x", md5res[i]);
            hex[MD5_DIGEST_LENGTH*2] = '\0';
            printf("MD5: %s\n", hex);
            fflush(stdout);
            close(outfd);
        }
    }

    if (logf) fclose(logf);
    close(sock);
    return 0;
}
