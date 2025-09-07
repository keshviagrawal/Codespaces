#define _POSIX_C_SOURCE 199309L
#include "sham.h"
#include <unistd.h>
#include <time.h> 

// Global variables
connection_t client_conn;
double loss_rate = 0.0;
int chat_mode = 0;
char *input_file = NULL;
char *output_file = NULL;

// Signal handler for cleanup
void signal_handler(int sig) {
    if (sig == SIGINT) {
        printf("\nShutting down client...\n");
        cleanup_connection(&client_conn);
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
void log_send_syn(connection_t *conn, uint32_t seq_num) {
    char event[100];
    snprintf(event, sizeof(event), "SND SYN SEQ=%u", seq_num);
    log_event(conn, event);
}

void log_rcv_syn_ack(connection_t *conn, uint32_t seq_num, uint32_t ack_num) {
    char event[100];
    snprintf(event, sizeof(event), "RCV SYN-ACK SEQ=%u ACK=%u", seq_num, ack_num);
    log_event(conn, event);
}

void log_send_ack_for_syn(connection_t *conn) {
    log_event(conn, "SND ACK FOR SYN");
}

void log_send_data(connection_t *conn, uint32_t seq_num, int len) {
    char event[100];
    snprintf(event, sizeof(event), "SND DATA SEQ=%u LEN=%d", seq_num, len);
    log_event(conn, event);
}

void log_rcv_ack(connection_t *conn, uint32_t ack_num) {
    char event[100];
    snprintf(event, sizeof(event), "RCV ACK=%u", ack_num);
    log_event(conn, event);
}

void log_timeout(connection_t *conn, uint32_t seq_num) {
    char event[100];
    snprintf(event, sizeof(event), "TIMEOUT SEQ=%u", seq_num);
    log_event(conn, event);
}

void log_retx_data(connection_t *conn, uint32_t seq_num, int len) {
    char event[100];
    snprintf(event, sizeof(event), "RETX DATA SEQ=%u LEN=%d", seq_num, len);
    log_event(conn, event);
}

void log_send_fin(connection_t *conn, uint32_t seq_num) {
    char event[100];
    snprintf(event, sizeof(event), "SND FIN SEQ=%u", seq_num);
    log_event(conn, event);
}

void log_rcv_ack_for_fin(connection_t *conn) {
    log_event(conn, "RCV ACK FOR FIN");
}

void log_rcv_fin(connection_t *conn, uint32_t seq_num) {
    char event[100];
    snprintf(event, sizeof(event), "RCV FIN SEQ=%u", seq_num);
    log_event(conn, event);
}

void log_send_ack_for_fin(connection_t *conn) {
    log_event(conn, "SND ACK FOR FIN");
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

// Three-way handshake (client side)
int three_way_handshake_client(connection_t *conn) {
    sham_packet_t packet;
    
    // Send SYN
    uint32_t client_seq = get_random_seq_num();
    send_packet(conn, client_seq, 0, SYN_FLAG, conn->window_size, NULL, 0);
    log_send_syn(conn, client_seq);
    conn->next_seq_num = client_seq + 1;
    conn->state = SYN_SENT;
    
    // Wait for SYN-ACK
    while (1) {
        int bytes_received = receive_packet(conn, &packet);
        if (bytes_received > 0 && (packet.header.flags & (SYN_FLAG | ACK_FLAG)) == (SYN_FLAG | ACK_FLAG)) {
            log_rcv_syn_ack(conn, packet.header.seq_num, packet.header.ack_num);
            conn->expected_seq_num = packet.header.seq_num + 1;
            conn->state = ESTABLISHED;
            break;
        }
    }
    
    // Send ACK
    send_packet(conn, 0, conn->expected_seq_num, ACK_FLAG, conn->window_size, NULL, 0);
    log_send_ack_for_syn(conn);
    
    printf("Connection established!\n");
    return 1;
}

// Four-way handshake close (client side)
int four_way_handshake_close(connection_t *conn) {
    sham_packet_t packet;
    
    // Send FIN
    uint32_t fin_seq = conn->next_seq_num;
    send_packet(conn, fin_seq, 0, FIN_FLAG, conn->window_size, NULL, 0);
    log_send_fin(conn, fin_seq);
    conn->state = FIN_WAIT_1;
    
    // Wait for ACK
    while (1) {
        int bytes_received = receive_packet(conn, &packet);
        if (bytes_received > 0 && (packet.header.flags & ACK_FLAG)) {
            log_rcv_ack_for_fin(conn);
            conn->state = FIN_WAIT_2;
            break;
        }
    }
    
    // Wait for FIN
    while (1) {
        int bytes_received = receive_packet(conn, &packet);
        if (bytes_received > 0 && (packet.header.flags & FIN_FLAG)) {
            log_rcv_fin(conn, packet.header.seq_num);
            conn->state = TIME_WAIT;
            break;
        }
    }
    
    // Send final ACK
    send_packet(conn, 0, packet.header.seq_num + 1, ACK_FLAG, conn->window_size, NULL, 0);
    log_send_ack_for_fin(conn);
    conn->state = CLOSED;
    
    // Wait a bit for the server to process the ACK
    // usleep()(100000); // 100ms

    struct timespec sleep_duration;
    sleep_duration.tv_sec = 0;
    sleep_duration.tv_nsec = 100000000; // 100 milliseconds
    nanosleep(&sleep_duration, NULL);
        
    printf("Connection closed!\n");
    return 1;
}

// Check if packet should be dropped
int should_drop_packet(double loss_rate) {
    if (loss_rate <= 0.0) return 0;
    return ((double)rand() / RAND_MAX) < loss_rate;
}

// Setup socket
int setup_socket(int port __attribute__((unused)), int is_server __attribute__((unused))) {
    int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd < 0) {
        perror("socket creation failed");
        return -1;
    }
    
    return socket_fd;
}

// Get random sequence number
uint32_t get_random_seq_num() {
    return (uint32_t)(rand() % 1000000) + 1000;
}

// File transfer mode
void handle_file_transfer(connection_t *conn) {
    FILE *file = fopen(input_file, "rb");
    if (!file) {
        perror("Failed to open input file");
        return;
    }
    
    char buffer[MAX_DATA_SIZE];
    int bytes_read;
    uint32_t current_seq = conn->next_seq_num;
    
    printf("Sending file: %s\n", input_file);
    
    while ((bytes_read = fread(buffer, 1, MAX_DATA_SIZE, file)) > 0) {
        // Send data packet
        send_packet(conn, current_seq, 0, 0, conn->window_size, buffer, bytes_read);
        log_send_data(conn, current_seq, bytes_read);
        
        // Wait for ACK with timeout
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = RTO_TIMEOUT;
        
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(conn->socket_fd, &read_fds);
        
        int result = select(conn->socket_fd + 1, &read_fds, NULL, NULL, &timeout);
        
        if (result > 0) {
            sham_packet_t ack_packet;
            int bytes_received = receive_packet(conn, &ack_packet);
            if (bytes_received > 0 && (ack_packet.header.flags & ACK_FLAG)) {
                log_rcv_ack(conn, ack_packet.header.ack_num);
                current_seq += bytes_read;
            }
        } else {
            // Timeout - retransmit
            log_timeout(conn, current_seq);
            log_retx_data(conn, current_seq, bytes_read);
            send_packet(conn, current_seq, 0, 0, conn->window_size, buffer, bytes_read);
            // Don't advance current_seq on retransmit
        }
    }
    
    fclose(file);
    printf("File sent successfully\n");
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
                    log_send_data(conn, conn->next_seq_num, len);
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
                    printf("Server: %.*s", data_len, packet.data);
                }
            }
        }
    }
}

// Parse command line arguments
int parse_arguments(int argc, char *argv[], char **server_ip, int *port, 
                   char **input_file, char **output_file, int *chat_mode, double *loss_rate) {
    if (argc < 3) {
        printf("Usage: %s <server_ip> <server_port> [input_file] [output_file] [--chat] [loss_rate]\n", argv[0]);
        return 0;
    }
    
    *server_ip = argv[1];
    *port = atoi(argv[2]);
    *chat_mode = 0;
    *loss_rate = 0.0;
    
    if (argc >= 4 && strcmp(argv[3], "--chat") == 0) {
        *chat_mode = 1;
        if (argc >= 5) {
            *loss_rate = atof(argv[4]);
        }
    } else if (argc >= 5) {
        *input_file = argv[3];
        *output_file = argv[4];
        if (argc >= 6) {
            if (strcmp(argv[5], "--chat") == 0) {
                *chat_mode = 1;
                if (argc >= 7) {
                    *loss_rate = atof(argv[6]);
                }
            } else {
                *loss_rate = atof(argv[5]);
            }
        }
    }
    
    return 1;
}

// Main function
int main(int argc, char *argv[]) {
    char *server_ip;
    int port;
    
    // Parse arguments
    if (!parse_arguments(argc, argv, &server_ip, &port, &input_file, &output_file, &chat_mode, &loss_rate)) {
        return 1;
    }
    
    // Setup signal handler
    signal(SIGINT, signal_handler);
    
    // Setup socket
    int socket_fd = setup_socket(port, 0);
    if (socket_fd < 0) {
        return 1;
    }
    
    // Setup server address
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("Invalid server IP address");
        close(socket_fd);
        return 1;
    }
    
    // Initialize connection
    init_connection(&client_conn, socket_fd, server_addr);
    
    // Setup logging
    setup_logging(&client_conn, "client_log.txt");
    
    printf("Connecting to server %s:%d\n", server_ip, port);
    if (chat_mode) {
        printf("Chat mode enabled\n");
    } else {
        printf("File transfer mode: %s -> %s\n", input_file, output_file);
    }
    if (loss_rate > 0) {
        printf("Packet loss rate: %.2f%%\n", loss_rate * 100);
    }
    
    // Establish connection
    if (three_way_handshake_client(&client_conn)) {
        if (chat_mode) {
            handle_chat_mode(&client_conn);
        } else {
            handle_file_transfer(&client_conn);
        }
        
        // Close connection
        four_way_handshake_close(&client_conn);
    }
    
    cleanup_connection(&client_conn);
    return 0;
}
