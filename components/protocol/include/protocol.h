/*
 * protocol - IGNACC ASCII line protocol (pure C, no ESP-IDF dependencies).
 *
 * Parses one received line (terminator already stripped) into a command and
 * formats the matching response. Every formatted response ends with "\r\n".
 *
 * The protocol knows the command grammar, not how many relays a board has:
 * IN<n> accepts n = 1..PROTO_MAX_CHANNELS and the caller decides whether that
 * channel exists.
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Highest channel number the grammar accepts (IN1..IN16). */
#define PROTO_MAX_CHANNELS 16

/** Buffer size sufficient for any response, including a 16-channel STATUS. */
#define PROTO_RESPONSE_MAX 160

typedef enum {
    PROTO_CMD_NONE = 0, /*!< Not a command: must be ignored silently */
    PROTO_CMD_RELAY,    /*!< IN<n>_ON / IN<n>_OFF: one channel */
    PROTO_CMD_ALL,      /*!< ALL_ON / ALL_OFF: every channel */
    PROTO_CMD_HELLO,    /*!< "HeyU" */
    PROTO_CMD_STATUS,
} proto_cmd_kind_t;

typedef struct {
    proto_cmd_kind_t kind;
    unsigned channel; /*!< PROTO_CMD_RELAY: 1..PROTO_MAX_CHANNELS, else 0 */
    bool on;          /*!< PROTO_CMD_RELAY / PROTO_CMD_ALL: requested state */
} proto_cmd_t;

/**
 * Parse a line. Matching is exact and case-sensitive; the line must not
 * contain the terminator. Channel numbers are plain decimal without leading
 * zeros ("IN1_ON", "IN12_OFF"; not "IN01_ON" or "IN0_ON"). Embedded NUL bytes
 * never match a command.
 *
 * @return the command; kind is PROTO_CMD_NONE for anything else (including NULL).
 */
proto_cmd_t proto_parse(const char *line, size_t len);

/*
 * Formatters write a complete response plus a NUL terminator into buf.
 * They return the response length (excluding NUL), or -1 on bad arguments or
 * if buf is too small.
 */

/**
 * Result of a relay command:
 *   PROTO_CMD_RELAY -> "IN<n>_ON_OK\r\n", "IN<n>_OFF_FAIL\r\n", ...
 *   PROTO_CMD_ALL   -> "ALL_ON_OK\r\n", "ALL_OFF_FAIL\r\n", ...
 */
int proto_format_result(const proto_cmd_t *cmd, bool ok, char *buf, size_t size);

/** "Hello IGNACC v<fw_version> READY\r\n" */
int proto_format_hello(const char *fw_version, char *buf, size_t size);

/**
 * "STATUS IN1=<ON|OFF> IN2=<ON|OFF> ... IN<count>=<ON|OFF>\r\n"
 * count must be 1..PROTO_MAX_CHANNELS.
 */
int proto_format_status(const bool *on, unsigned count, char *buf, size_t size);

#ifdef __cplusplus
}
#endif
