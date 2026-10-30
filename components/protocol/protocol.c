#include "protocol.h"

#include <stdio.h>
#include <string.h>

typedef struct {
    const char *text;
    proto_cmd_t cmd;
} fixed_cmd_t;

/* Commands without parameters. */
static const fixed_cmd_t s_fixed[] = {
    { "ALL_ON",  { PROTO_CMD_ALL,    0, true  } },
    { "ALL_OFF", { PROTO_CMD_ALL,    0, false } },
    { "HeyU",    { PROTO_CMD_HELLO,  0, false } },
    { "STATUS",  { PROTO_CMD_STATUS, 0, false } },
};

static const proto_cmd_t s_none = { PROTO_CMD_NONE, 0, false };

static bool equals(const char *line, size_t len, const char *text)
{
    return strlen(text) == len && memcmp(text, line, len) == 0;
}

/* "IN<n>_ON" / "IN<n>_OFF", n = 1..PROTO_MAX_CHANNELS without leading zeros. */
static proto_cmd_t parse_relay(const char *line, size_t len)
{
    if (len < 3 || line[0] != 'I' || line[1] != 'N' || line[2] < '1' || line[2] > '9') {
        return s_none;
    }
    size_t i = 2;
    unsigned channel = 0;
    while (i < len && line[i] >= '0' && line[i] <= '9') {
        channel = channel * 10u + (unsigned)(line[i] - '0');
        if (channel > PROTO_MAX_CHANNELS) {
            return s_none;
        }
        i++;
    }
    const char *rest = line + i;
    size_t rest_len = len - i;
    if (equals(rest, rest_len, "_ON")) {
        return (proto_cmd_t){ PROTO_CMD_RELAY, channel, true };
    }
    if (equals(rest, rest_len, "_OFF")) {
        return (proto_cmd_t){ PROTO_CMD_RELAY, channel, false };
    }
    return s_none;
}

proto_cmd_t proto_parse(const char *line, size_t len)
{
    if (line == NULL) {
        return s_none;
    }
    for (size_t i = 0; i < sizeof(s_fixed) / sizeof(s_fixed[0]); i++) {
        if (equals(line, len, s_fixed[i].text)) {
            return s_fixed[i].cmd;
        }
    }
    return parse_relay(line, len);
}

/* snprintf wrapper: returns length, or -1 if truncated or failed. */
static int format_checked(size_t size, int n)
{
    if (n < 0 || (size_t)n >= size) {
        return -1;
    }
    return n;
}

int proto_format_result(const proto_cmd_t *cmd, bool ok, char *buf, size_t size)
{
    if (cmd == NULL || buf == NULL || size == 0) {
        return -1;
    }
    const char *state = cmd->on ? "ON" : "OFF";
    const char *result = ok ? "OK" : "FAIL";
    int n;

    switch (cmd->kind) {
    case PROTO_CMD_RELAY:
        if (cmd->channel < 1 || cmd->channel > PROTO_MAX_CHANNELS) {
            return -1;
        }
        n = snprintf(buf, size, "IN%u_%s_%s\r\n", cmd->channel, state, result);
        break;
    case PROTO_CMD_ALL:
        n = snprintf(buf, size, "ALL_%s_%s\r\n", state, result);
        break;
    default:
        return -1;
    }
    return format_checked(size, n);
}

int proto_format_hello(const char *fw_version, char *buf, size_t size)
{
    if (fw_version == NULL || buf == NULL || size == 0) {
        return -1;
    }
    int n = snprintf(buf, size, "Hello IGNACC v%s READY\r\n", fw_version);
    return format_checked(size, n);
}

int proto_format_status(const bool *on, unsigned count, char *buf, size_t size)
{
    if (on == NULL || count < 1 || count > PROTO_MAX_CHANNELS || buf == NULL || size == 0) {
        return -1;
    }
    size_t used = 0;
    int n = snprintf(buf, size, "STATUS");
    if (format_checked(size, n) < 0) {
        return -1;
    }
    used = (size_t)n;
    for (unsigned i = 0; i < count; i++) {
        n = snprintf(buf + used, size - used, " IN%u=%s", i + 1u, on[i] ? "ON" : "OFF");
        if (format_checked(size - used, n) < 0) {
            return -1;
        }
        used += (size_t)n;
    }
    n = snprintf(buf + used, size - used, "\r\n");
    if (format_checked(size - used, n) < 0) {
        return -1;
    }
    return (int)(used + (size_t)n);
}
