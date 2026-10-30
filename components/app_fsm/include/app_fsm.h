/**
 * @file app_fsm.h
 * @brief Application state machine: the sole owner of the relays.
 *
 * Pure C. Consumes received lines, drives relays and sends responses through
 * the injected link. All dependencies (relays, link, delay) are injected, so
 * it runs unchanged on the host with mocks, and with any number of relays
 * (1..#PROTO_MAX_CHANNELS) of any #relay_t implementation.
 *
 * States:
 * @verbatim
 *   INIT  --app_fsm_start()-->  READY
 *   INIT  : lines are ignored.
 *   READY : lines are parsed and executed.
 * @endverbatim
 *
 * Command handling in READY:
 * - `IN<n>_ON/OFF`: drive relay n, wait the settle delay, verify the read-back,
 *   reply `_OK`. A channel the board doesn't have is ignored.
 * - `ALL_ON/OFF`: drive every relay, wait **one** settle delay, verify every
 *   read-back, reply `_OK`.
 * - Any failure: reply `_FAIL` immediately and drive **all** relays OFF.
 *
 * Safety: all relays are driven OFF on start and whenever a relay command fails.
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "link_if.h"
#include "protocol.h"
#include "relay_bank.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Delay function injected into the FSM (vTaskDelay on target, a fake in tests).
 * @param ctx Caller context (app_fsm_config_t::delay_ctx).
 * @param ms  Milliseconds to wait.
 */
typedef void (*app_fsm_delay_fn_t)(void *ctx, uint32_t ms);

/** FSM states. */
typedef enum {
    APP_FSM_STATE_INIT = 0, /**< Created; lines are ignored. */
    APP_FSM_STATE_READY,    /**< Started; lines are executed. */
} app_fsm_state_t;

/** FSM configuration. */
typedef struct {
    relay_bank_t relays;     /**< IN1..INn, 1..#PROTO_MAX_CHANNELS relays. The handles are copied at create. */
    link_tx_t link;          /**< Where responses are sent. */
    const char *fw_version;  /**< Reported by HeyU. Must outlive the FSM. */
    uint32_t settle_ms;      /**< Wait between driving relays and sending `_OK`. 0 = no wait. */
    app_fsm_delay_fn_t delay; /**< Delay implementation. */
    void *delay_ctx;         /**< Passed to @ref delay. */
} app_fsm_config_t;

/** Opaque FSM instance. */
typedef struct app_fsm app_fsm_t;

/**
 * @brief Create an FSM in state #APP_FSM_STATE_INIT.
 *
 * The relay handles are copied, so `cfg->relays.relays` may be a temporary
 * array. The relays themselves are not touched until app_fsm_start().
 *
 * @param cfg Configuration.
 * @return The FSM, or NULL if @p cfg is invalid (missing link, version, delay
 *         or relays; count outside 1..#PROTO_MAX_CHANNELS) or memory runs out.
 */
app_fsm_t *app_fsm_create(const app_fsm_config_t *cfg);

/**
 * @brief Free an FSM. The relays are left as they are. Safe to call with NULL.
 * @param fsm The FSM.
 */
void app_fsm_destroy(app_fsm_t *fsm);

/**
 * @brief Go from INIT to READY, driving all relays OFF first.
 * @param fsm The FSM.
 * @return true if every relay was driven OFF successfully. The FSM is READY
 *         either way.
 */
bool app_fsm_start(app_fsm_t *fsm);

/**
 * @brief Handle one received line. Unknown lines are ignored silently.
 * @param fsm  The FSM.
 * @param line Line bytes, terminator stripped; need not be NUL-terminated.
 * @param len  Number of bytes in @p line.
 */
void app_fsm_handle_line(app_fsm_t *fsm, const char *line, size_t len);

/**
 * @brief Current state.
 * @param fsm The FSM.
 * @return #APP_FSM_STATE_INIT or #APP_FSM_STATE_READY.
 */
app_fsm_state_t app_fsm_state(const app_fsm_t *fsm);

/**
 * @brief Number of relay channels this FSM controls.
 * @param fsm The FSM.
 * @return 1..#PROTO_MAX_CHANNELS.
 */
unsigned app_fsm_channel_count(const app_fsm_t *fsm);

#ifdef __cplusplus
}
#endif
