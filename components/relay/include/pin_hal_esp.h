/**
 * @file pin_hal_esp.h
 * @brief #pin_hal_t implementation on top of the ESP-IDF GPIO driver.
 *
 * pin_hal_esp.c is the only file in the project that includes driver/gpio.h.
 */
#pragma once

#include "pin_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief The ESP-IDF GPIO HAL instance.
 *
 * Stateless: `ctx` is NULL and the same instance can be shared by all relays.
 * configure_output() writes the level before calling gpio_config() with
 * GPIO_MODE_INPUT_OUTPUT, which satisfies the no-glitch contract.
 *
 * @return The HAL instance.
 */
pin_hal_t pin_hal_esp(void);

#ifdef __cplusplus
}
#endif
