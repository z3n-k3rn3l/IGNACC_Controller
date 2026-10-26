#include "protocol.h"

#include <stdio.h>
#include <string.h>

typedef struct {
    const char *text;
    proto_cmd_t cmd;
} cmd_entry_t;

static const cmd_entry_t s_commands[] = {
    { "IN1_ON",  PROTO_CMD_IN1_ON  },
    { "IN1_OFF", PROTO_CMD_IN1_OFF },
    { "IN2_ON",  PROTO_CMD_IN2_ON  },
    { "IN2_OFF", PROTO_CMD_IN2_OFF },
    { "HeyU",    PROTO_CMD_HELLO   },
    { "STATUS",  PROTO_CMD_STATUS  },
};

proto_cmd_t proto_parse(const char *line, size_t len)
{
    if (line == NULL) {
        return PROTO_CMD_NONE;
    }
    for (size_t i = 0; i < sizeof(s_commands) / sizeof(s_commands[0]); i++) {
        const char *text = s_commands[i].text;
        if (strlen(text) == len && memcmp(text, line, len) == 0) {
            return s_commands[i].cmd;
        }
    }
    return PROTO_CMD_NONE;
}

bool proto_cmd_relay_target(proto_cmd_t cmd, unsigned *channel, bool *on)
{
    unsigned ch;
    bool state;

    switch (cmd) {
    case PROTO_CMD_IN1_ON:  ch = 0; state = true;  break;
    case PROTO_CMD_IN1_OFF: ch = 0; state = false; break;
    case PROTO_CMD_IN2_ON:  ch = 1; state = true;  break;
    case PROTO_CMD_IN2_OFF: ch = 1; state = false; break;
    default:
        return false;
    }
    if (channel != NULL) {
        *channel = ch;
    }
    if (on != NULL) {
        *on = state;
    }
    return true;
}

/* snprintf wrapper: returns length, or -1 if truncated or failed. */
static int format_checked(size_t size, int n)
{
    if (n < 0 || (size_t)n >= size) {
        return -1;
    }
    return n;
}

int proto_format_relay_result(proto_cmd_t cmd, bool ok, char *buf, size_t size)
{
    unsigned channel;
    bool on;

    if (buf == NULL || size == 0 || !proto_cmd_relay_target(cmd, &channel, &on)) {
        return -1;
    }
    int n = snprintf(buf, size, "IN%u_%s_%s\r\n", channel + 1u, on ? "ON" : "OFF", ok ? "OK" : "FAIL");
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

int proto_format_status(bool in1_on, bool in2_on, char *buf, size_t size)
{
    if (buf == NULL || size == 0) {
        return -1;
    }
    int n = snprintf(buf, size, "STATUS IN1=%s IN2=%s\r\n", in1_on ? "ON" : "OFF", in2_on ? "ON" : "OFF");
    return format_checked(size, n);
}
