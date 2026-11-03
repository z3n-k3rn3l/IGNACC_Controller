/**
 * @file app_fsm.c
 * @brief The state machine: parses each line, drives relays, sends replies.
 *
 * Tour stop 4, the heart of the firmware. Read app_fsm_handle_line() first,
 * then handle_command(), which dispatches to the handlers above it. Every
 * relay failure ends in drive_safe_state() (all relays OFF).
 *
 * @see docs/diagrams/state-app-fsm.svg
 * @see docs/diagrams/activity-command-handling.svg
 */
#include "app_fsm.h"

#include <stdlib.h>
#include <string.h>

/** FSM instance (opaque to users of app_fsm.h). */
struct app_fsm {
    app_fsm_config_t cfg;  /**< Copy of the config; cfg.relays points at @ref relays. */
    app_fsm_state_t state; /**< INIT or READY. */
    relay_t relays[];      /**< Owned copy of the relay handles (flexible array). */
};

/**
 * @brief Send a formatted reply if formatting succeeded.
 *
 * Link errors are ignored: nothing useful can be done, and the host times out.
 *
 * @param self The FSM.
 * @param buf  Reply text.
 * @param len  Length from a proto_format_*() call; nothing is sent if <= 0.
 */
static void send_response(app_fsm_t *self, const char *buf, int len)
{
    if (len > 0) {
        /* Nothing useful can be done if the link fails: the host times out. */
        (void)link_tx_send(&self->cfg.link, buf, (size_t)len);
    }
}

/**
 * @brief Drive every relay OFF, even if one of them fails.
 * @param self The FSM.
 * @return true if every relay was driven OFF successfully.
 */
static bool drive_safe_state(app_fsm_t *self)
{
    return relay_bank_set_all(&self->cfg.relays, false) == RELAY_OK;
}

/**
 * @brief Wait the relay settle delay (skipped when settle_ms is 0).
 * @param self The FSM.
 */
static void settle(app_fsm_t *self)
{
    if (self->cfg.settle_ms > 0) {
        self->cfg.delay(self->cfg.delay_ctx, self->cfg.settle_ms);
    }
}

/**
 * @brief `IN<n>_ON` / `IN<n>_OFF`: drive, settle, verify, reply.
 *
 * On any failure all relays are driven OFF and the reply is `_FAIL`, sent
 * immediately (no settle delay).
 *
 * @param self The FSM.
 * @param cmd  A #PROTO_CMD_RELAY command whose channel exists on this board.
 */
static void handle_relay_command(app_fsm_t *self, const proto_cmd_t *cmd)
{
    const relay_bank_t *bank = &self->cfg.relays;
    unsigned index = cmd->channel - 1u;
    char buf[PROTO_RESPONSE_MAX];

    bool ok = relay_bank_set(bank, index, cmd->on) == RELAY_OK;
    if (ok) {
        settle(self);
        ok = relay_bank_is_on(bank, index) == cmd->on;
    }
    if (!ok) {
        (void)drive_safe_state(self);
    }
    send_response(self, buf, proto_format_result(cmd, ok, buf, sizeof(buf)));
}

/**
 * @brief `ALL_ON` / `ALL_OFF`: drive every relay, settle once, verify all, reply.
 * @param self The FSM.
 * @param cmd  A #PROTO_CMD_ALL command.
 */
static void handle_all_command(app_fsm_t *self, const proto_cmd_t *cmd)
{
    const relay_bank_t *bank = &self->cfg.relays;
    char buf[PROTO_RESPONSE_MAX];

    bool ok = relay_bank_set_all(bank, cmd->on) == RELAY_OK;
    if (ok) {
        settle(self);
        ok = relay_bank_all_are(bank, cmd->on);
    }
    if (!ok) {
        (void)drive_safe_state(self);
    }
    send_response(self, buf, proto_format_result(cmd, ok, buf, sizeof(buf)));
}

/**
 * @brief `STATUS`: read every relay back and reply with one field per channel.
 * @param self The FSM.
 */
static void handle_status(app_fsm_t *self)
{
    const relay_bank_t *bank = &self->cfg.relays;
    bool states[PROTO_MAX_CHANNELS];
    char buf[PROTO_RESPONSE_MAX];

    for (unsigned i = 0; i < relay_bank_count(bank); i++) {
        states[i] = relay_bank_is_on(bank, i);
    }
    send_response(self, buf, proto_format_status(states, relay_bank_count(bank), buf, sizeof(buf)));
}

/**
 * @brief Dispatch one parsed command (READY state only).
 *
 * Unknown lines, and `IN<n>` for a channel this board doesn't have, are
 * answered with `UNK_NACK`; no relay is touched.
 *
 * @param self The FSM.
 * @param cmd  Result of proto_parse().
 */
static void handle_command(app_fsm_t *self, const proto_cmd_t *cmd)
{
    char buf[PROTO_RESPONSE_MAX];

    switch (cmd->kind) {
    case PROTO_CMD_RELAY:
        if (cmd->channel >= 1 && cmd->channel <= relay_bank_count(&self->cfg.relays)) {
            handle_relay_command(self, cmd);
        } else {
            /* A channel this board doesn't have: unknown here. */
            send_response(self, buf, proto_format_nack(buf, sizeof(buf)));
        }
        break;
    case PROTO_CMD_ALL:
        handle_all_command(self, cmd);
        break;
    case PROTO_CMD_HELLO:
        send_response(self, buf, proto_format_hello(self->cfg.fw_version, buf, sizeof(buf)));
        break;
    case PROTO_CMD_STATUS:
        handle_status(self);
        break;
    case PROTO_CMD_NONE:
    default:
        send_response(self, buf, proto_format_nack(buf, sizeof(buf)));
        break;
    }
}

app_fsm_t *app_fsm_create(const app_fsm_config_t *cfg)
{
    if (cfg == NULL || cfg->link.ops == NULL || cfg->fw_version == NULL || cfg->delay == NULL ||
        cfg->relays.relays == NULL || cfg->relays.count < 1 || cfg->relays.count > PROTO_MAX_CHANNELS) {
        return NULL;
    }
    for (unsigned i = 0; i < cfg->relays.count; i++) {
        if (cfg->relays.relays[i].ops == NULL) {
            return NULL;
        }
    }

    app_fsm_t *self = malloc(sizeof(*self) + cfg->relays.count * sizeof(relay_t));
    if (self == NULL) {
        return NULL;
    }
    memcpy(self->relays, cfg->relays.relays, cfg->relays.count * sizeof(relay_t));
    self->cfg = *cfg;
    self->cfg.relays.relays = self->relays;
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
    case APP_FSM_STATE_READY: {
        proto_cmd_t cmd = proto_parse(line, len);
        handle_command(fsm, &cmd);
        break;
    }
    case APP_FSM_STATE_INIT:
    default:
        break;
    }
}

app_fsm_state_t app_fsm_state(const app_fsm_t *fsm)
{
    return fsm->state;
}

unsigned app_fsm_channel_count(const app_fsm_t *fsm)
{
    return relay_bank_count(&fsm->cfg.relays);
}
