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
