/*
 * app_fsm - application state machine. Pure C.
 *
 * Sole owner of the relays. Consumes received lines, drives relays and sends
 * responses through the injected link. All dependencies (relays, link, delay)
 * are injected, so it runs unchanged on the host with mocks.
 *
 * States:
 *   INIT  --app_fsm_start()-->  READY
 *   INIT  : lines are ignored.
 *   READY : lines are parsed and executed.
 *
 * Safety: on start, and whenever a relay command fails, both relays are
 * driven OFF.
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "link_if.h"
#include "protocol.h"
#include "relay.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*app_fsm_delay_fn_t)(void *ctx, uint32_t ms);

typedef enum {
    APP_FSM_STATE_INIT = 0,
    APP_FSM_STATE_READY,
} app_fsm_state_t;

typedef struct {
    relay_t relays[PROTO_RELAY_CHANNELS]; /*!< [0] = IN1, [1] = IN2 */
    link_tx_t link;
    const char *fw_version;               /*!< Must outlive the FSM */
    uint32_t settle_ms;                   /*!< Delay before _OK is sent */
    app_fsm_delay_fn_t delay;
    void *delay_ctx;
} app_fsm_config_t;

typedef struct app_fsm app_fsm_t;

/** Create in state INIT. Returns NULL on invalid config or no memory. */
app_fsm_t *app_fsm_create(const app_fsm_config_t *cfg);

void app_fsm_destroy(app_fsm_t *fsm);

/**
 * INIT -> READY. Drives both relays OFF first.
 * @return true if the safe state was applied and verified.
 */
bool app_fsm_start(app_fsm_t *fsm);

/** Handle one received line (terminator stripped). Unknown lines are ignored. */
void app_fsm_handle_line(app_fsm_t *fsm, const char *line, size_t len);

app_fsm_state_t app_fsm_state(const app_fsm_t *fsm);

#ifdef __cplusplus
}
#endif
