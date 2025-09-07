#include "sham.h"

// Global variables
connection_t server_conn;
double loss_rate = 0.0;
int chat_mode = 0;

// Signal handler for cleanup
void signal_handler(int sig) {
    if (sig == SIGINT) {
        printf("\nShutting down server...\n");
        cleanup_connection(&server_conn);
        exit(0);
    }
}

// Initialize connection
void init_connection(connection_t *conn, int socket_fd, struct sockaddr_in addr) {
    conn->socket_fd = socket_fd;
    conn->addr = addr;
    conn->state = CLOSED;
    conn->next_seq_num = get_random_seq_num();
    conn->expected_seq_num = 0;
    conn->window_size = 8192;
    conn->timer_count = 0;
    conn->log_file = NULL;
    conn->is_logging = 0;
    
    // Initialize timers
    for (int i = 0; i < WINDOW_SIZE; i++) {
        conn->timers[i].seq_num = 0;
        conn->timers[i].retries = 0;
    }
}

// Cleanup connection
void cleanup_connection(connection_t *conn) {
    if (conn->log_file) {
        fclose(conn->log_file);
        conn->log_file = NULL;
    }
    if (conn->socket_fd > 0) {
        close(conn->socket_fd);
    }
}

// Setup logging
void setup_logging(connection_t *conn, const char *log_filename) {
    if (getenv("RUDP_LOG") && strcmp(getenv("RUDP_LOG"), "1") == 0) {
        conn->log_file = fopen(log_filename, "w");
        if (conn->log_file) {
            conn->is_logging = 1;
        }
    }
}

// Log event with timestamp
void log_event(connection_t *conn, const char *event) {
    if (!conn->is_logging || !conn->log_file) return;
    
    char time_buffer[30];
    struct timeval tv;
    time_t curtime;
    
    gettimeofday(&tv, NULL);
    curtime = tv.tv_sec;
    strftime(time_buffer, 30, "%Y-%m-%d %H:%M:%S", localtime(&curtime));
    
    fprintf(conn->log_file, "[%s.%06d] [LOG] %s\n", time_buffer, (int)tv.tv_usec, event);
    fflush(conn->log_file);
}

// Specific log functions
void log_rcv_syn(connection_t *conn, uint32_t seq_num) {
    char event[100];
    snprintf(event, sizeof(event), "RCV SYN SEQ=%u", seq_num);
    log_event(conn, event);
}

void log_send_syn_ack(connection_t *conn, uint32_t seq_num, uint32_t ack_num) {
    char event[100];
    snprintf(event, sizeof(event), "SND SYN-ACK SEQ=%u ACK=%u", seq_num, ack_num);
    log_event(conn, event);
}

void log_rcv_ack_for_syn(connection_t *conn) {
    log_event(conn, "RCV ACK FOR SYN");
}

void log_rcv_data(connection_t *conn, uint32_t seq_num, int len) {
    char event[100];
    snprintf(event, sizeof(event), "RCV DATA SEQ=%u LEN=%d", seq_num, len);
    log_event(conn, event);
}

void log_send_ack(connection_t *conn, uint32_t ack_num, uint16_t window_size) {
    char event[100];
    snprintf(event, sizeof(event), "SND ACK=%u WIN=%u", ack_num, window_size);
    log_event(conn, event);
}

void log_rcv_fin(connection_t *conn, uint32_t seq_num) {
    char event[100];
    snprintf(event, sizeof(event), "RCV FIN SEQ=%u", seq_num);
    log_event(conn, event);
}

void log_send_ack_for_fin(connection_t *conn) {
    log_event(conn, "SND ACK FOR FIN");
}

void log_send_fin(connection_t *conn, uint32_t seq_num) {
    char event[100];
    snprintf(event, sizeof(event), "SND FIN SEQ=%u", seq_num);
    log_event(conn, event);
}

void log_rcv_ack_for_fin(connection_t *conn) {
    log_event(conn, "RCV ACK FOR FIN");
}

void log_drop_data(connection_t *conn, uint32_t seq_num) {
    char event[100];
    snprintf(event, sizeof(event), "DROP DATA SEQ=%u", seq_num);
    log_event(conn, event);
}

// Send packet
int send_packet(connection_t *conn, uint32_t seq_num, uint32_t ack_num, 
                uint16_t flags, uint16_t window_size, const char *data, int data_len) {
    sham_packet_t packet;
    
    packet.header.seq_num = htonl(seq_num);
    packet.header.ack_num = htonl(ack_num);
    packet.header.flags = htons(flags);
    packet.header.window_size = htons(window_size);
    
    if (data && data_len > 0) {
        memcpy(packet.data, data, data_len);
    }
    
    int bytes_sent = sendto(conn->socket_fd, &packet, HEADER_SIZE + data_len, 0,
                           (struct sockaddr*)&conn->addr, sizeof(conn->addr));
    
    return bytes_sent;
}

// Receive packet
int receive_packet(connection_t *conn, sham_packet_t *packet) {
    socklen_t addr_len = sizeof(conn->addr);
    int bytes_received = recvfrom(conn->socket_fd, packet, sizeof(sham_packet_t), 0,
                                 (struct sockaddr*)&conn->addr, &addr_len);
    
    if (bytes_received > 0) {
        // Convert from network byte order
        packet->header.seq_num = ntohl(packet->header.seq_num);
        packet->header.ack_num = ntohl(packet->header.ack_num);
        packet->header.flags = ntohs(packet->header.flags);
        packet->header.window_size = ntohs(packet->header.window_size);
    }
    
    return bytes_received;
}

// Three-way handshake (server side)
int three_way_handshake_server(connection_t *conn) {
    sham_packet_t packet;
    
    // Wait for SYN
    printf("Waiting for client connection...\n");
    while (1) {
        int bytes_received = receive_packet(conn, &packet);
        if (bytes_received > 0 && (packet.header.flags & SYN_FLAG)) {
            log_rcv_syn(conn, packet.header.seq_num);
            conn->expected_seq_num = packet.header.seq_num + 1;
            conn->state = SYN_RCVD;
            break;
        }
    }
    
    // Send SYN-ACK
    uint32_t server_seq = get_random_seq_num();
    send_packet(conn, server_seq, conn->expected_seq_num, 
                SYN_FLAG | ACK_FLAG, conn->window_size, NULL, 0);
    log_send_syn_ack(conn, server_seq, conn->expected_seq_num);
    conn->next_seq_num = server_seq + 1;
    
    // Wait for ACK
    while (1) {
        int bytes_received = receive_packet(conn, &packet);
        if (bytes_received > 0 && (packet.header.flags & ACK_FLAG) && 
            packet.header.ack_num == conn->next_seq_num) {
            log_rcv_ack_for_syn(conn);
            conn->state = ESTABLISHED;
            printf("Connection established!\n");
            return 1;
        }
    }
}

// Four-way handshake close
int four_way_handshake_close(connection_t *conn) {
    sham_packet_t packet;
    
    // Wait for FIN
    while (1) {
        int bytes_received = receive_packet(conn, &packet);
        if (bytes_received > 0 && (packet.header.flags & FIN_FLAG)) {
            log_rcv_fin(conn, packet.header.seq_num);
            conn->state = CLOSE_WAIT;
            break;
        }
    }
    
    // Send ACK for FIN
    send_packet(conn, 0, packet.header.seq_num + 1, ACK_FLAG, conn->window_size, NULL, 0);
    log_send_ack_for_fin(conn);
    
    // Send our own FIN
    send_packet(conn, conn->next_seq_num, 0, FIN_FLAG, conn->window_size, NULL, 0);
    log_send_fin(conn, conn->next_seq_num);
    conn->state = LAST_ACK;
    
    // Wait for final ACK with timeout
    struct timeval timeout;
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;
    
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(conn->socket_fd, &read_fds);
    
    int result = select(conn->socket_fd + 1, &read_fds, NULL, NULL, &timeout);
    
    if (result > 0) {
        int bytes_received = receive_packet(conn, &packet);
        if (bytes_received > 0 && (packet.header.flags & ACK_FLAG) && 
            packet.header.ack_num == conn->next_seq_num + 1) {
            log_rcv_ack_for_fin(conn);
            conn->state = CLOSED;
            printf("Connection closed!\n");
            return 1;
        }
    }
    
    // Timeout - assume connection is closed
    conn->state = CLOSED;
    printf("Connection closed (timeout)!\n");
    return 1;
}

// Check if packet should be dropped
int should_drop_packet(double loss_rate) {
    if (loss_rate <= 0.0) return 0;
    return ((double)rand() / RAND_MAX) < loss_rate;
}

// Calculate MD5 hash
void calculate_md5(const char *filename, char *md5_hash) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        strcpy(md5_hash, "error");
        return;
    }
    
    MD5_CTX md5_context;
    MD5_Init(&md5_context);
    
    char buffer[1024];
    int bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        MD5_Update(&md5_context, buffer, bytes_read);
    }
    
    unsigned char hash[MD5_DIGEST_LENGTH];
    MD5_Final(hash, &md5_context);
    
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
        sprintf(md5_hash + (i * 2), "%02x", hash[i]);
    }
    md5_hash[32] = '\0';
    
    fclose(file);
}

// Setup socket
int setup_socket(int port, int is_server) {
    int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd < 0) {
        perror("socket creation failed");
        return -1;
    }
    
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    
    if (is_server) {
        addr.sin_addr.s_addr = INADDR_ANY;
        if (bind(socket_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            perror("bind failed");
            close(socket_fd);
            return -1;
        }
    }
    
    return socket_fd;
}

// Get random sequence number
uint32_t get_random_seq_num() {
    return (uint32_t)(rand() % 1000000) + 1000;
}

// File transfer mode
void handle_file_transfer(connection_t *conn) {
    FILE *output_file = fopen("received_file", "wb");
    if (!output_file) {
        perror("Failed to create output file");
        return;
    }
    
    sham_packet_t packet;
    int total_bytes = 0;
    
    printf("Receiving file...\n");
    
    while (1) {
        int bytes_received = receive_packet(conn, &packet);
        if (bytes_received <= 0) continue;
        
        // Check for FIN
        if (packet.header.flags & FIN_FLAG) {
            log_rcv_fin(conn, packet.header.seq_num);
            break;
        }
        
        // Check if packet should be dropped
        if (should_drop_packet(loss_rate)) {
            log_drop_data(conn, packet.header.seq_num);
            continue;
        }
        
        // Process data
        int data_len = bytes_received - HEADER_SIZE;
        if (data_len > 0) {
            log_rcv_data(conn, packet.header.seq_num, data_len);
            fwrite(packet.data, 1, data_len, output_file);
            total_bytes += data_len;
            
            // Send ACK
            send_packet(conn, 0, packet.header.seq_num + data_len, 
                       ACK_FLAG, conn->window_size, NULL, 0);
            log_send_ack(conn, packet.header.seq_num + data_len, conn->window_size);
        }
    }
    
    fclose(output_file);
    printf("File received successfully (%d bytes)\n", total_bytes);
    
    // Calculate and print MD5
    char md5_hash[33];
    calculate_md5("received_file", md5_hash);
    printf("MD5: %s\n", md5_hash);
}

// Chat mode
void handle_chat_mode(connection_t *conn) {
    printf("Chat mode activated. Type '/quit' to exit.\n");
    
    fd_set read_fds;
    char input_buffer[1024];
    sham_packet_t packet;
    
    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(0, &read_fds);  // stdin
        FD_SET(conn->socket_fd, &read_fds);
        
        int max_fd = (conn->socket_fd > 0) ? conn->socket_fd : 0;
        select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        
        // Handle stdin input
        if (FD_ISSET(0, &read_fds)) {
            if (fgets(input_buffer, sizeof(input_buffer), stdin)) {
                if (strncmp(input_buffer, "/quit", 5) == 0) {
                    break;
                }
                
                // Send message
                int len = strlen(input_buffer);
                if (len > 0) {
                    send_packet(conn, conn->next_seq_num, 0, 0, conn->window_size, 
                               input_buffer, len);
                    conn->next_seq_num += len;
                }
            }
        }
        
        // Handle network input
        if (FD_ISSET(conn->socket_fd, &read_fds)) {
            int bytes_received = receive_packet(conn, &packet);
            if (bytes_received > 0) {
                if (packet.header.flags & FIN_FLAG) {
                    break;
                }
                
                int data_len = bytes_received - HEADER_SIZE;
                if (data_len > 0) {
                    printf("Client: %.*s", data_len, packet.data);
                }
            }
        }
    }
}

// Main function
int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <port> [--chat] [loss_rate]\n", argv[0]);
        return 1;
    }
    
    int port = atoi(argv[1]);
    loss_rate = 0.0;
    chat_mode = 0;
    
    // Parse arguments
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--chat") == 0) {
            chat_mode = 1;
        } else if (atof(argv[i]) > 0) {
            loss_rate = atof(argv[i]);
        }
    }
    
    // Setup signal handler
    signal(SIGINT, signal_handler);
    
    // Setup socket
    int socket_fd = setup_socket(port, 1);
    if (socket_fd < 0) {
        return 1;
    }
    
    // Initialize connection
    struct sockaddr_in client_addr = {0};
    init_connection(&server_conn, socket_fd, client_addr);
    
    // Setup logging
    setup_logging(&server_conn, "server_log.txt");
    
    printf("Server started on port %d\n", port);
    if (chat_mode) {
        printf("Chat mode enabled\n");
    }
    if (loss_rate > 0) {
        printf("Packet loss rate: %.2f%%\n", loss_rate * 100);
    }
    
    // Wait for connection
    if (three_way_handshake_server(&server_conn)) {
        if (chat_mode) {
            handle_chat_mode(&server_conn);
        } else {
            handle_file_transfer(&server_conn);
        }
        
        // Close connection
        four_way_handshake_close(&server_conn);
    }
    
    cleanup_connection(&server_conn);
    return 0;
}
