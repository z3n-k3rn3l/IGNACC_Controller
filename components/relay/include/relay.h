/**
 * @file relay.h
 * @brief Abstract relay interface: an opaque handle plus a function table.
 *
 * Every relay implementation (relay_gpio today; an I/O expander or shift
 * register tomorrow) fills in a #relay_ops_t and keeps its state behind the
 * opaque `ctx` pointer. Users (relay_bank, app_fsm) only see this header, so a
 * new implementation never requires changes to them.
 */
#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Result of a relay operation. */
typedef enum {
    RELAY_OK = 0,     /**< Success. */
    RELAY_ERR_ARG,    /**< Invalid argument or configuration. */
    RELAY_ERR_NOMEM,  /**< Allocation failed. */
    RELAY_ERR_IO,     /**< The underlying pin API reported an error. */
    RELAY_ERR_VERIFY, /**< Read-back did not match the commanded state. */
} relay_err_t;

/** Operations every relay implementation provides. */
typedef struct {
    /**
     * Drive the relay ON or OFF and verify the drive.
     * @param ctx Implementation handle (relay_t::ctx).
     * @param on  true = ON, false = OFF.
     * @return #RELAY_OK, or an error.
     */
    relay_err_t (*set)(void *ctx, bool on);
    /**
     * Current state as read back from the hardware.
     * @param ctx Implementation handle (relay_t::ctx).
     * @return true if the relay reads as ON.
     */
    bool (*is_on)(void *ctx);
} relay_ops_t;

/** A relay: function table plus opaque implementation handle. */
typedef struct {
    const relay_ops_t *ops; /**< Implementation's function table. */
    void *ctx;              /**< Opaque implementation handle. */
} relay_t;

/**
 * @brief Drive a relay ON or OFF.
 * @param relay The relay.
 * @param on    true = ON, false = OFF.
 * @return #RELAY_OK, or the implementation's error.
 */
static inline relay_err_t relay_set(const relay_t *relay, bool on)
{
    return relay->ops->set(relay->ctx, on);
}

/**
 * @brief Read a relay's state back from the hardware.
 * @param relay The relay.
 * @return true if the relay reads as ON.
 */
static inline bool relay_is_on(const relay_t *relay)
{
    return relay->ops->is_on(relay->ctx);
}

#ifdef __cplusplus
}
#endif
