// // sham.h

// #ifndef SHAM_H
// #define SHAM_H

// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <stdint.h>
// #include <arpa/inet.h>
// #include <sys/time.h>

// #define SYN 0x1
// #define ACK 0x2
// #define FIN 0x4

// #define BUFFER_SIZE 1024
// #define WINDOW_SIZE 10
// #define RTO_TIMEOUT 500000 // 500ms in microseconds
// #define PACKET_LOSS_RATE 0.1 // 10%

// // S.H.A.M. Header Structure
// // All fields are in network byte order
// typedef struct {
//     uint32_t seq_num;     // Sequence Number
//     uint32_t ack_num;     // Acknowledgment Number
//     uint16_t flags;       // Control flags (SYN, ACK, FIN)
//     uint16_t window_size; // Flow control window size
// } sham_header;

// // Function to pack the header and payload into a single buffer
// void pack_sham_packet(char* buffer, const sham_header* header, const char* payload, size_t payload_len);

// // Function to unpack the header from a received buffer
// void unpack_sham_packet(const char* buffer, sham_header* header, char* payload, size_t* payload_len);

// // Utility function for time-based retransmission
// long get_time_diff(struct timeval start, struct timeval end);

// #endif // SHAM_H

// sham.h

#ifndef SHAM_H
#define SHAM_H

#include <stdint.h>
#include <sys/time.h>
#include <sys/types.h>
#include <netinet/in.h>

// Packet and Protocol Constants
#define BUFFER_SIZE 1024
#define RTO_TIMEOUT_MS 500
#define SLIDING_WINDOW_SIZE 10

// S.H.A.M. Control Flags (16-bit field)
#define SYN 0x1  // Synchronise (Connection initiation)
#define ACK 0x2  // Acknowledge (Acknowledgment number is valid)
#define FIN 0x4  // Finish (Connection termination)

// Recommended S.H.A.M. Header Structure
typedef struct {
    uint32_t seq_num;     // Sequence Number
    uint32_t ack_num;     // Acknowledgment Number
    uint16_t flags;       // Control flags (SYN, ACK, FIN)
    uint16_t window_size; // Flow control window size (in bytes)
} sham_header;

// Functions for converting header to/from network byte order
void pack_sham_packet(char* buffer, const sham_header* header, const char* payload, size_t payload_len);
void unpack_sham_packet(const char* buffer, sham_header* header, char* payload, size_t* payload_len);

// Utility function for time difference calculation
long get_time_diff_ms(struct timeval start, struct timeval end);

#endif // SHAM_H
