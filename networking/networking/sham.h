#ifndef SHAM_H
#define SHAM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <openssl/md5.h>
#include <signal.h>

// S.H.A.M. Header Structure
struct sham_header {
    uint32_t seq_num;      // Sequence Number
    uint32_t ack_num;      // Acknowledgment Number
    uint16_t flags;        // Control flags (SYN, ACK, FIN)
    uint16_t window_size;  // Flow control window size
} __attribute__((packed));

// Flag definitions
#define SYN_FLAG 0x1
#define ACK_FLAG 0x2
#define FIN_FLAG 0x4

// Protocol constants
#define MAX_PACKET_SIZE 1024
#define HEADER_SIZE sizeof(struct sham_header)
#define MAX_DATA_SIZE (MAX_PACKET_SIZE - HEADER_SIZE)
#define WINDOW_SIZE 10
#define RTO_TIMEOUT 500000  // 500ms in microseconds
#define MAX_RETRIES 5

// Connection states
typedef enum {
    CLOSED,
    LISTEN,
    SYN_SENT,
    SYN_RCVD,
    ESTABLISHED,
    FIN_WAIT_1,
    FIN_WAIT_2,
    CLOSE_WAIT,
    CLOSING,
    LAST_ACK,
    TIME_WAIT
} connection_state_t;

// Packet structure
typedef struct {
    struct sham_header header;
    char data[MAX_DATA_SIZE];
} sham_packet_t;

// Timer structure for retransmission
typedef struct {
    uint32_t seq_num;
    struct timeval send_time;
    int retries;
    sham_packet_t packet;
} packet_timer_t;

// Connection structure
typedef struct {
    int socket_fd;
    struct sockaddr_in addr;
    connection_state_t state;
    uint32_t next_seq_num;
    uint32_t expected_seq_num;
    uint16_t window_size;
    packet_timer_t timers[WINDOW_SIZE];
    int timer_count;
    FILE *log_file;
    int is_logging;
} connection_t;

// Function declarations
void init_connection(connection_t *conn, int socket_fd, struct sockaddr_in addr);
void cleanup_connection(connection_t *conn);
int send_packet(connection_t *conn, uint32_t seq_num, uint32_t ack_num, 
                uint16_t flags, uint16_t window_size, const char *data, int data_len);
int receive_packet(connection_t *conn, sham_packet_t *packet);
int three_way_handshake_client(connection_t *conn);
int three_way_handshake_server(connection_t *conn);
int four_way_handshake_close(connection_t *conn);
int send_data(connection_t *conn, const char *data, int data_len);
int receive_data(connection_t *conn, char *buffer, int buffer_size);
void setup_logging(connection_t *conn, const char *log_filename);
void log_event(connection_t *conn, const char *event);
void log_send_syn(connection_t *conn, uint32_t seq_num);
void log_rcv_syn(connection_t *conn, uint32_t seq_num);
void log_send_syn_ack(connection_t *conn, uint32_t seq_num, uint32_t ack_num);
void log_rcv_ack_for_syn(connection_t *conn);
void log_send_data(connection_t *conn, uint32_t seq_num, int len);
void log_rcv_data(connection_t *conn, uint32_t seq_num, int len);
void log_send_ack(connection_t *conn, uint32_t ack_num, uint16_t window_size);
void log_rcv_ack(connection_t *conn, uint32_t ack_num);
void log_timeout(connection_t *conn, uint32_t seq_num);
void log_retx_data(connection_t *conn, uint32_t seq_num, int len);
void log_flow_win_update(connection_t *conn, uint16_t new_window_size);
void log_drop_data(connection_t *conn, uint32_t seq_num);
void log_send_fin(connection_t *conn, uint32_t seq_num);
void log_rcv_fin(connection_t *conn, uint32_t seq_num);
void log_send_ack_for_fin(connection_t *conn);
void log_rcv_ack_for_fin(connection_t *conn);
int should_drop_packet(double loss_rate);
void calculate_md5(const char *filename, char *md5_hash);
int setup_socket(int port, int is_server);
void handle_timeout(connection_t *conn);
int add_timer(connection_t *conn, uint32_t seq_num, sham_packet_t packet);
void remove_timer(connection_t *conn, uint32_t seq_num);
void update_timers(connection_t *conn);

// Utility functions
uint32_t get_random_seq_num();
void print_packet_info(const sham_packet_t *packet, const char *direction);
int parse_arguments(int argc, char *argv[], char **server_ip, int *port, 
                   char **input_file, char **output_file, int *chat_mode, double *loss_rate);

#endif // SHAM_H
