/*
 * relay_gpio - relay_t implementation for a relay driven by one digital pin.
 * Pure C: all pin access goes through the injected pin_hal_t.
 */
#pragma once

#include "pin_hal.h"
#include "relay.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int pin;
    int active_level; /*!< Pin level that switches the relay ON (0 = active-low) */
} relay_gpio_config_t;

/**
 * Create a relay and put it in the safe state (OFF): the OFF level is latched
 * before the pin becomes an output, then verified by read-back.
 *
 * @param cfg  pin configuration
 * @param hal  pin HAL; copied, must outlive the relay's use of hal->ctx
 * @param out  receives the relay handle on success
 */
relay_err_t relay_gpio_create(const relay_gpio_config_t *cfg, const pin_hal_t *hal, relay_t *out);

/** Release a relay created by relay_gpio_create. The pin is left as is. */
void relay_gpio_destroy(relay_t *relay);

#ifdef __cplusplus
}
#endif
