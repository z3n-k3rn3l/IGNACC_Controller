/**
 * @file relay_gpio.h
 * @brief #relay_t implementation for a relay driven by one digital pin.
 *
 * Pure C: all pin access goes through the injected #pin_hal_t, so the driver
 * runs on the host with a mock HAL. Each relay is verified by reading the pin
 * back after every drive. This checks the pin, not the relay contact.
 */
#pragma once

#include "pin_hal.h"
#include "relay.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Pin configuration of one GPIO relay. */
typedef struct {
    int pin;          /**< GPIO number. */
    int active_level; /**< Pin level that switches the relay ON: 0 = active-low, 1 = active-high. */
} relay_gpio_config_t;

/**
 * @brief Create a GPIO relay and put it in the safe state (OFF).
 *
 * The OFF level is latched before the pin becomes an output, so the relay
 * never glitches ON at boot. The OFF level is then verified by read-back.
 *
 * @param cfg Pin configuration.
 * @param hal Pin HAL. Copied; `hal->ctx` must outlive the relay.
 * @param out Receives the relay handle on success.
 * @return #RELAY_OK; #RELAY_ERR_ARG for NULL or invalid arguments;
 *         #RELAY_ERR_NOMEM; #RELAY_ERR_IO if the pin could not be configured;
 *         #RELAY_ERR_VERIFY if the pin did not read back as OFF.
 */
relay_err_t relay_gpio_create(const relay_gpio_config_t *cfg, const pin_hal_t *hal, relay_t *out);

/**
 * @brief Release a relay created by relay_gpio_create().
 *
 * The pin is left in its current state. Safe to call with NULL.
 *
 * @param relay The relay; its fields are cleared.
 */
void relay_gpio_destroy(relay_t *relay);

#ifdef __cplusplus
}
#endif
