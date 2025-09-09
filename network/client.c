// // client.c

// #include "sham.h"
// #include <unistd.h>
// #include <sys/socket.h>
// #include <netinet/in.h>
// #include <fcntl.h>
// #include <time.h>

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
//         // In this simple case, we assume the rest is payload
//         // A real protocol would have a length field
//     }
// }

// long get_time_diff(struct timeval start, struct timeval end) {
//     return (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
// }

// // Global variables for simplicity in this example
// int sockfd;
// struct sockaddr_in servaddr;
// socklen_t len = sizeof(servaddr);

// // --- Connection Management ---
// int three_way_handshake() {
//     printf("CLIENT: Starting three-way handshake...\n");
    
//     // Step 1: Client -> Server (SYN)
//     sham_header syn_header = { .seq_num = 12345, .ack_num = 0, .flags = SYN, .window_size = 0 };
//     char packet[BUFFER_SIZE + sizeof(sham_header)];
//     pack_sham_packet(packet, &syn_header, NULL, 0);
//     sendto(sockfd, packet, sizeof(sham_header), 0, (struct sockaddr*)&servaddr, len);
//     printf("CLIENT: Sent SYN with SEQ=%u\n", syn_header.seq_num);

//     // Step 2: Server -> Client (SYN-ACK)
//     struct timeval timeout = {0, 500000}; // 500ms
//     setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
//     ssize_t n = recvfrom(sockfd, packet, sizeof(packet), 0, (struct sockaddr*)&servaddr, &len);
//     if (n <= 0) {
//         printf("CLIENT: Timeout waiting for SYN-ACK\n");
//         return -1;
//     }
//     sham_header syn_ack_header;
//     unpack_sham_packet(packet, &syn_ack_header, NULL, NULL);
//     if ((syn_ack_header.flags & (SYN | ACK)) != (SYN | ACK) || syn_ack_header.ack_num != syn_header.seq_num + 1) {
//         printf("CLIENT: Invalid SYN-ACK received\n");
//         return -1;
//     }
//     printf("CLIENT: Received SYN-ACK. Server SEQ=%u, ACK=%u\n", syn_ack_header.seq_num, syn_ack_header.ack_num);

//     // Step 3: Client -> Server (ACK)
//     sham_header ack_header = { .seq_num = syn_header.seq_num + 1, .ack_num = syn_ack_header.seq_num + 1, .flags = ACK, .window_size = 0 };
//     pack_sham_packet(packet, &ack_header, NULL, 0);
//     sendto(sockfd, packet, sizeof(sham_header), 0, (struct sockaddr*)&servaddr, len);
//     printf("CLIENT: Sent ACK with ACK=%u\n", ack_header.ack_num);
//     printf("CLIENT: Handshake complete. Connection established.\n");
//     return 0;
// }

// void four_way_handshake() {
//     printf("CLIENT: Initiating four-way handshake...\n");

//     // Step 1: Client -> Server (FIN)
//     sham_header fin_header = { .flags = FIN };
//     char packet[sizeof(sham_header)];
//     pack_sham_packet(packet, &fin_header, NULL, 0);
//     sendto(sockfd, packet, sizeof(sham_header), 0, (struct sockaddr*)&servaddr, len);
//     printf("CLIENT: Sent FIN\n");

//     // Step 2: Server -> Client (ACK)
//     struct timeval timeout = {1, 0};
//     setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
//     ssize_t n = recvfrom(sockfd, packet, sizeof(packet), 0, (struct sockaddr*)&servaddr, &len);
//     if (n <= 0) {
//         printf("CLIENT: Timeout or error waiting for ACK. Aborting termination.\n");
//         return;
//     }
//     sham_header ack_header;
//     unpack_sham_packet(packet, &ack_header, NULL, NULL);
//     if (!(ack_header.flags & ACK)) {
//         printf("CLIENT: Invalid ACK received.\n");
//         return;
//     }
//     printf("CLIENT: Received ACK for FIN.\n");

//     // Step 3: Server -> Client (FIN)
//     n = recvfrom(sockfd, packet, sizeof(packet), 0, (struct sockaddr*)&servaddr, &len);
//     if (n <= 0) {
//         printf("CLIENT: Timeout or error waiting for final FIN. Aborting termination.\n");
//         return;
//     }
//     sham_header fin_header2;
//     unpack_sham_packet(packet, &fin_header2, NULL, NULL);
//     if (!(fin_header2.flags & FIN)) {
//         printf("CLIENT: Invalid FIN received.\n");
//         return;
//     }
//     printf("CLIENT: Received server's FIN.\n");

//     // Step 4: Client -> Server (ACK)
//     sham_header final_ack_header = { .flags = ACK };
//     pack_sham_packet(packet, &final_ack_header, NULL, 0);
//     sendto(sockfd, packet, sizeof(sham_header), 0, (struct sockaddr*)&servaddr, len);
//     printf("CLIENT: Sent final ACK. Connection closed.\n");
// }

// // --- Main Logic ---
// int main(int argc, char *argv[]) {
//     if (argc != 4) {
//         fprintf(stderr, "Usage: %s <server_ip> <server_port> <filename>\n", argv[0]);
//         exit(1);
//     }

//     char *server_ip = argv[1];
//     int server_port = atoi(argv[2]);
//     char *filename = argv[3];

//     sockfd = socket(AF_INET, SOCK_DGRAM, 0);
//     if (sockfd < 0) {
//         perror("socket creation failed");
//         exit(EXIT_FAILURE);
//     }

//     memset(&servaddr, 0, sizeof(servaddr));
//     servaddr.sin_family = AF_INET;
//     servaddr.sin_port = htons(server_port);
//     inet_pton(AF_INET, server_ip, &servaddr.sin_addr);

//     if (three_way_handshake() != 0) {
//         close(sockfd);
//         return 1;
//     }
    
//     // Simulating file transfer logic
//     FILE* fp = fopen(filename, "rb");
//     if (!fp) {
//         perror("Failed to open file");
//         close(sockfd);
//         return 1;
//     }

//     printf("CLIENT: Starting file transfer of '%s'...\n", filename);

//     ssize_t bytes_read;
//     char buffer[BUFFER_SIZE];
//     long total_sent = 0;
//     uint32_t base_seq = 12346;
    
//     while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, fp)) > 0) {
//         sham_header data_header = { .seq_num = base_seq + total_sent, .ack_num = 0, .flags = ACK, .window_size = 0 };
//         char packet[sizeof(sham_header) + BUFFER_SIZE];
//         pack_sham_packet(packet, &data_header, buffer, bytes_read);
        
//         // This is a simplified send loop. A full sliding window would
//         // not block here, but would send multiple packets before waiting.
//         // It would also need a retransmission timer.
//         sendto(sockfd, packet, sizeof(sham_header) + bytes_read, 0, (struct sockaddr*)&servaddr, len);
//         printf("CLIENT: Sent data with SEQ=%u\n", data_header.seq_num);
//         total_sent += bytes_read;
        
//         // Wait for ACK
//         sham_header ack_header;
//         struct timeval timeout = {0, RTO_TIMEOUT};
//         setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
//         ssize_t n = recvfrom(sockfd, packet, sizeof(packet), 0, (struct sockaddr*)&servaddr, &len);
//         if (n <= 0) {
//             printf("CLIENT: ACK timeout. Retransmitting last packet...\n");
//             // Simplified retransmission: a real impl would re-send the entire window
//             sendto(sockfd, packet, sizeof(sham_header) + bytes_read, 0, (struct sockaddr*)&servaddr, len);
//         } else {
//             unpack_sham_packet(packet, &ack_header, NULL, NULL);
//             printf("CLIENT: Received ACK with ACK=%u\n", ack_header.ack_num);
//         }
//     }
    
//     printf("CLIENT: File transfer complete.\n");
//     fclose(fp);
//     four_way_handshake();
//     close(sockfd);
//     return 0;
// }
