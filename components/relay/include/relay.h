/*
 * relay - abstract relay interface (opaque handle + function table).
 *
 * Consumers only see this header. Implementations (e.g. relay_gpio) fill in
 * the ops table and keep their state behind the opaque ctx pointer.
 */
#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    RELAY_OK = 0,
    RELAY_ERR_ARG,    /*!< Invalid argument or configuration */
    RELAY_ERR_NOMEM,  /*!< Allocation failed */
    RELAY_ERR_IO,     /*!< Underlying pin API reported an error */
    RELAY_ERR_VERIFY, /*!< Pin read-back did not match the commanded level */
} relay_err_t;

typedef struct {
    /** Drive the relay ON or OFF and verify the drive. */
    relay_err_t (*set)(void *ctx, bool on);
    /** Current state as read back from the hardware. */
    bool (*is_on)(void *ctx);
} relay_ops_t;

typedef struct {
    const relay_ops_t *ops;
    void *ctx; /*!< Opaque implementation handle */
} relay_t;

static inline relay_err_t relay_set(const relay_t *relay, bool on)
{
    return relay->ops->set(relay->ctx, on);
}

static inline bool relay_is_on(const relay_t *relay)
{
    return relay->ops->is_on(relay->ctx);
}

#ifdef __cplusplus
}
#endif
