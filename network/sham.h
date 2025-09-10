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




// #ifndef SHAM_H
// #define SHAM_H

// #include <stdint.h>
// #include <sys/time.h>

// #define SHAM_DATA_SIZE 1024
// #define SHAM_SENDER_WINDOW_PKTS 10
// #define SHAM_RTO_MS 500

// // Flags
// #define SHAM_SYN 0x1
// #define SHAM_ACK 0x2
// #define SHAM_FIN 0x4

// // On-wire header (network byte order for fields)
// #pragma pack(push, 1)
// struct sham_header {
//     uint32_t seq_num;     // byte-based sequence of first data byte (or SYN/FIN)
//     uint32_t ack_num;     // next expected byte (cumulative ACK)
//     uint16_t flags;       // SHAM_SYN, SHAM_ACK, SHAM_FIN
//     uint16_t window_size; // receiver advertised free bytes
// };
// #pragma pack(pop)

// // Utility: ms since epoch
// static inline uint64_t sham_now_ms(void) {
//     struct timeval tv; gettimeofday(&tv, NULL);
//     return (uint64_t)tv.tv_sec * 1000ULL + (uint64_t)tv.tv_usec / 1000ULL;
// }

// #endif



#ifndef SHAM_H
#define SHAM_H

#include <stdint.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>

#define SHAM_DATA_SIZE 1024
#define SHAM_SENDER_WINDOW_PKTS 10
#define SHAM_RTO_MS 500

// Flags
#define SHAM_SYN 0x1
#define SHAM_ACK 0x2
#define SHAM_FIN 0x4

// On-wire header (network byte order for fields)
#pragma pack(push, 1)
struct sham_header {
    uint32_t seq_num;     // byte-based sequence of first data byte (or SYN/FIN)
    uint32_t ack_num;     // next expected byte (cumulative ACK)
    uint16_t flags;       // SHAM_SYN, SHAM_ACK, SHAM_FIN
    uint16_t window_size; // receiver advertised free bytes
};
#pragma pack(pop)

// Utility: ms since epoch
static inline uint64_t sham_now_ms(void) {
    struct timeval tv; gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000ULL + (uint64_t)tv.tv_usec / 1000ULL;
}

// -------- Verbose logging (RUDP_LOG=1 enables) --------
static inline FILE* rudp_log_open(const char* role_filename) {
    const char* env = getenv("RUDP_LOG");
    if (!env || strcmp(env, "1") != 0) return NULL;
    FILE* f = fopen(role_filename, "w");
    return f;
}

static inline void rudp_logf(FILE* f, const char* fmt, ...) {
    if (!f) return;
    char time_buffer[32];
    struct timeval tv;
    gettimeofday(&tv, NULL);
    time_t curtime = tv.tv_sec;
    struct tm tmv; localtime_r(&curtime, &tmv);
    strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", &tmv);
    fprintf(f, "[%s.%06ld] [LOG] ", time_buffer, (long)tv.tv_usec);
    va_list ap; va_start(ap, fmt);
    vfprintf(f, fmt, ap);
    va_end(ap);
    fputc('\n', f);
    fflush(f);
}

// -------- Loss simulation helper (receiver only, for data) --------
static inline int rudp_should_drop(double loss_rate) {
    if (loss_rate <= 0.0) return 0;
    if (loss_rate >= 1.0) return 1;
    // rand() seeded in main()
    double r = (double)rand() / (double)RAND_MAX;
    return (r < loss_rate);
}

#endif
