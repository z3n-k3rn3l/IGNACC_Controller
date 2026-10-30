/**
 * @file protocol.h
 * @brief IGNACC ASCII line protocol: parse commands, format responses.
 *
 * Pure C, no ESP-IDF dependencies, fully host-testable.
 *
 * The protocol knows the command *grammar*, not the board. `IN<n>` accepts
 * n = 1..#PROTO_MAX_CHANNELS; the caller (app_fsm) decides whether channel n
 * exists on this board and ignores it otherwise. This is what lets the channel
 * count be a build setting without touching this module.
 *
 * | Command                  | Response                                     |
 * |--------------------------|----------------------------------------------|
 * | `IN<n>_ON` / `IN<n>_OFF` | `IN<n>_ON_OK` / `IN<n>_ON_FAIL` / ...        |
 * | `ALL_ON` / `ALL_OFF`     | `ALL_ON_OK` / `ALL_ON_FAIL` / ...            |
 * | `HeyU`                   | `Hello IGNACC v<version> READY`              |
 * | `STATUS`                 | `STATUS IN1=<ON|OFF> ... IN<count>=<ON|OFF>` |
 * | anything else            | no response                                  |
 *
 * Received lines have their terminator stripped before parsing. Every
 * formatted response ends with `"\r\n"`.
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Highest channel number the grammar accepts (IN1..IN16). */
#define PROTO_MAX_CHANNELS 16

/** Buffer size sufficient for any response, including a 16-channel STATUS line. */
#define PROTO_RESPONSE_MAX 160

/** What kind of command a line is. */
typedef enum {
    PROTO_CMD_NONE = 0, /**< Not a command: must be ignored silently. */
    PROTO_CMD_RELAY,    /**< `IN<n>_ON` / `IN<n>_OFF`: one channel. */
    PROTO_CMD_ALL,      /**< `ALL_ON` / `ALL_OFF`: every channel on the board. */
    PROTO_CMD_HELLO,    /**< `HeyU`: handshake. */
    PROTO_CMD_STATUS,   /**< `STATUS`: report every channel. */
} proto_cmd_kind_t;

/** A parsed command. */
typedef struct {
    proto_cmd_kind_t kind; /**< Command kind. */
    unsigned channel;      /**< #PROTO_CMD_RELAY: 1..#PROTO_MAX_CHANNELS; 0 otherwise. */
    bool on;               /**< #PROTO_CMD_RELAY / #PROTO_CMD_ALL: requested state. */
} proto_cmd_t;

/**
 * @brief Parse one received line.
 *
 * Matching is exact and case-sensitive. Channel numbers are plain decimal
 * without leading zeros: `IN1_ON` and `IN12_OFF` are valid, `IN01_ON`,
 * `IN0_ON` and `IN17_ON` are not. Embedded NUL bytes never match.
 *
 * @param line Received bytes, terminator already stripped. Need not be
 *             NUL-terminated. May be NULL.
 * @param len  Number of bytes in @p line.
 * @return The command. `kind` is #PROTO_CMD_NONE for anything that is not a
 *         valid command (including a NULL @p line).
 */
proto_cmd_t proto_parse(const char *line, size_t len);

/**
 * @brief Format the result of a relay command.
 *
 * - #PROTO_CMD_RELAY: `"IN<n>_ON_OK\r\n"`, `"IN<n>_OFF_FAIL\r\n"`, ...
 * - #PROTO_CMD_ALL:   `"ALL_ON_OK\r\n"`, `"ALL_OFF_FAIL\r\n"`, ...
 *
 * @param cmd  The command being answered (#PROTO_CMD_RELAY or #PROTO_CMD_ALL).
 * @param ok   true for `_OK`, false for `_FAIL`.
 * @param buf  Output buffer; receives the response and a NUL terminator.
 * @param size Size of @p buf in bytes.
 * @return Response length excluding the NUL, or -1 if @p cmd is not a relay
 *         command, its channel is out of range, or @p buf is NULL or too small.
 */
int proto_format_result(const proto_cmd_t *cmd, bool ok, char *buf, size_t size);

/**
 * @brief Format the handshake reply `"Hello IGNACC v<fw_version> READY\r\n"`.
 *
 * @param fw_version Firmware version string, e.g. "1.1.0".
 * @param buf        Output buffer; receives the response and a NUL terminator.
 * @param size       Size of @p buf in bytes.
 * @return Response length excluding the NUL, or -1 on NULL arguments or if
 *         @p buf is too small.
 */
int proto_format_hello(const char *fw_version, char *buf, size_t size);

/**
 * @brief Format `"STATUS IN1=<ON|OFF> ... IN<count>=<ON|OFF>\r\n"`.
 *
 * @param on    Array of @p count states; `on[0]` is IN1.
 * @param count Number of channels, 1..#PROTO_MAX_CHANNELS.
 * @param buf   Output buffer; receives the response and a NUL terminator.
 *              #PROTO_RESPONSE_MAX bytes always suffice.
 * @param size  Size of @p buf in bytes.
 * @return Response length excluding the NUL, or -1 on NULL arguments, a
 *         @p count out of range, or a too-small @p buf.
 */
int proto_format_status(const bool *on, unsigned count, char *buf, size_t size);

#ifdef __cplusplus
}
#endif
