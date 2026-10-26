/*
 * protocol - IGNACC ASCII line protocol (pure C, no ESP-IDF dependencies).
 *
 * Parses one received line (terminator already stripped) into a command and
 * formats the matching response. Every formatted response ends with "\r\n".
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Buffer size sufficient for any response (version string up to 32 chars). */
#define PROTO_RESPONSE_MAX 64

/** Number of relay channels addressed by the protocol (IN1, IN2). */
#define PROTO_RELAY_CHANNELS 2

typedef enum {
    PROTO_CMD_NONE = 0, /*!< Not a command: must be ignored silently */
    PROTO_CMD_IN1_ON,
    PROTO_CMD_IN1_OFF,
    PROTO_CMD_IN2_ON,
    PROTO_CMD_IN2_OFF,
    PROTO_CMD_HELLO,    /*!< "HeyU" */
    PROTO_CMD_STATUS,
} proto_cmd_t;

/**
 * Parse a line. Matching is exact and case-sensitive; the line must not
 * contain the terminator. Embedded NUL bytes never match a command.
 *
 * @return the command, or PROTO_CMD_NONE for anything else (including NULL).
 */
proto_cmd_t proto_parse(const char *line, size_t len);

/**
 * For a relay command, report which channel (0 = IN1, 1 = IN2) and which
 * state it requests.
 *
 * @return true if cmd is a relay command, false otherwise (outputs untouched).
 */
bool proto_cmd_relay_target(proto_cmd_t cmd, unsigned *channel, bool *on);

/*
 * Formatters write a complete response plus a NUL terminator into buf.
 * They return the response length (excluding NUL), or -1 on bad arguments or
 * if buf is too small.
 */

/** "IN1_ON_OK\r\n", "IN2_OFF_FAIL\r\n", ... cmd must be a relay command. */
int proto_format_relay_result(proto_cmd_t cmd, bool ok, char *buf, size_t size);

/** "Hello IGNACC v<fw_version> READY\r\n" */
int proto_format_hello(const char *fw_version, char *buf, size_t size);

/** "STATUS IN1=<ON|OFF> IN2=<ON|OFF>\r\n" */
int proto_format_status(bool in1_on, bool in2_on, char *buf, size_t size);

#ifdef __cplusplus
}
#endif
