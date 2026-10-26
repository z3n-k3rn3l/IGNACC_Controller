#include "app_fsm.h"

#include <stdlib.h>

struct app_fsm {
    app_fsm_config_t cfg;
    app_fsm_state_t state;
};

static void send_response(app_fsm_t *self, const char *buf, int len)
{
    if (len > 0) {
        /* Nothing useful can be done if the link fails: the host times out. */
        (void)link_tx_send(&self->cfg.link, buf, (size_t)len);
    }
}

/* Drive every relay OFF, even if one of them fails. */
static bool drive_safe_state(app_fsm_t *self)
{
    bool ok = true;
    for (unsigned i = 0; i < PROTO_RELAY_CHANNELS; i++) {
        if (relay_set(&self->cfg.relays[i], false) != RELAY_OK) {
            ok = false;
        }
    }
    return ok;
}

static void handle_relay_command(app_fsm_t *self, proto_cmd_t cmd)
{
    unsigned channel;
    bool on;
    char buf[PROTO_RESPONSE_MAX];

    (void)proto_cmd_relay_target(cmd, &channel, &on);
    const relay_t *relay = &self->cfg.relays[channel];

    bool ok = relay_set(relay, on) == RELAY_OK;
    if (ok) {
        if (self->cfg.settle_ms > 0) {
            self->cfg.delay(self->cfg.delay_ctx, self->cfg.settle_ms);
        }
        /* The pin must still hold the commanded level after settling. */
        ok = relay_is_on(relay) == on;
    }
    if (!ok) {
        (void)drive_safe_state(self);
    }
    send_response(self, buf, proto_format_relay_result(cmd, ok, buf, sizeof(buf)));
}

static void handle_command(app_fsm_t *self, proto_cmd_t cmd)
{
    char buf[PROTO_RESPONSE_MAX];

    switch (cmd) {
    case PROTO_CMD_IN1_ON:
    case PROTO_CMD_IN1_OFF:
    case PROTO_CMD_IN2_ON:
    case PROTO_CMD_IN2_OFF:
        handle_relay_command(self, cmd);
        break;
    case PROTO_CMD_HELLO:
        send_response(self, buf, proto_format_hello(self->cfg.fw_version, buf, sizeof(buf)));
        break;
    case PROTO_CMD_STATUS:
        send_response(self, buf,
                      proto_format_status(relay_is_on(&self->cfg.relays[0]),
                                          relay_is_on(&self->cfg.relays[1]), buf, sizeof(buf)));
        break;
    case PROTO_CMD_NONE:
    default:
        break; /* Not a command: ignore silently. */
    }
}

app_fsm_t *app_fsm_create(const app_fsm_config_t *cfg)
{
    if (cfg == NULL || cfg->link.ops == NULL || cfg->fw_version == NULL || cfg->delay == NULL) {
        return NULL;
    }
    for (unsigned i = 0; i < PROTO_RELAY_CHANNELS; i++) {
        if (cfg->relays[i].ops == NULL) {
            return NULL;
        }
    }

    app_fsm_t *self = malloc(sizeof(*self));
    if (self == NULL) {
        return NULL;
    }
    self->cfg = *cfg;
    self->state = APP_FSM_STATE_INIT;
    return self;
}

void app_fsm_destroy(app_fsm_t *fsm)
{
    free(fsm);
}

bool app_fsm_start(app_fsm_t *fsm)
{
    bool ok = drive_safe_state(fsm);
    fsm->state = APP_FSM_STATE_READY;
    return ok;
}

void app_fsm_handle_line(app_fsm_t *fsm, const char *line, size_t len)
{
    switch (fsm->state) {
    case APP_FSM_STATE_READY:
        handle_command(fsm, proto_parse(line, len));
        break;
    case APP_FSM_STATE_INIT:
    default:
        break;
    }
}

app_fsm_state_t app_fsm_state(const app_fsm_t *fsm)
{
    return fsm->state;
}
