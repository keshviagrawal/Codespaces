#ifndef SHAM_MODIFIED_H
#define SHAM_MODIFIED_H

#include <stdint.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>
#include <string.h>

// --- Protocol Constants ---
#define SHAM_DATA_SIZE 1024
#define SHAM_SENDER_WINDOW_PKTS 10
#define SHAM_RTO_MS 500

// --- Control Flags ---
#define SHAM_SYN 0x1
#define SHAM_ACK 0x2
#define SHAM_FIN 0x4

// --- Magic Strings ---
#define FILENAME_BANNER "FNAME:"
#define CHAT_QUIT_COMMAND "/quit\n"

// --- Connection States ---
// Used by both client and server to track the connection status
typedef enum {
    S_CLOSED,
    S_LISTEN,       // Server only
    S_SYN_SENT,     // Client only
    S_SYN_RCVD,     // Server only
    S_ESTABLISHED,
    S_FIN_WAIT_1,   // Active close
    S_FIN_WAIT_2,
    S_CLOSING,      // Simultaneous close
    S_TIME_WAIT,
    S_CLOSE_WAIT,   // Passive close
    S_LAST_ACK,
} sham_state;


// On-wire header (network byte order for fields)
#pragma pack(push, 1)
struct sham_header {
    uint32_t seq_num;     // byte-based sequence of first data byte (or SYN/FIN)
    uint32_t ack_num;     // next expected byte (cumulative ACK)
    uint16_t flags;       // SHAM_SYN, SHAM_ACK, SHAM_FIN
    uint16_t window_size; // receiver advertised free bytes
};
#pragma pack(pop)

// --- Utility Functions ---

// Get current time in milliseconds since epoch
static inline uint64_t sham_now_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000ULL + (uint64_t)tv.tv_usec / 1000ULL;
}

// --- Verbose logging (RUDP_LOG=1 enables) ---
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
    struct tm tmv;
    localtime_r(&curtime, &tmv);
    strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", &tmv);
    fprintf(f, "[%s.%06ld] [LOG] ", time_buffer, (long)tv.tv_usec);
    va_list ap;
    va_start(ap, fmt);
    vfprintf(f, fmt, ap);
    va_end(ap);
    fputc('\n', f);
    fflush(f);
}

// --- Loss simulation helper (receiver only, for data) ---
static inline int rudp_should_drop(double loss_rate) {
    if (loss_rate <= 0.0) return 0;
    if (loss_rate >= 1.0) return 1;
    double r = (double)rand() / (double)RAND_MAX;
    return (r < loss_rate);
}

#endif // SHAM_MODIFIED_H
