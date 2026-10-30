/**
 * @file pin_hal.h
 * @brief Thin digital-pin HAL used by relay_gpio.
 *
 * Exists so the GPIO relay driver can be tested on the host with a mock.
 * The ESP-IDF implementation is pin_hal_esp().
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/** Pin operations. */
typedef struct {
    /**
     * Configure a pin as input+output, so its level can be read back.
     *
     * Contract: @p initial_level must be latched before the output driver is
     * enabled, so the pin never glitches to the other level.
     *
     * @param ctx           Implementation handle (pin_hal_t::ctx).
     * @param pin           GPIO number.
     * @param initial_level Level (0/1) the pin must have when it becomes an output.
     * @return 0 on success, non-zero on error.
     */
    int (*configure_output)(void *ctx, int pin, int initial_level);
    /**
     * Drive a pin.
     * @param ctx   Implementation handle.
     * @param pin   GPIO number.
     * @param level 0 or 1.
     * @return 0 on success, non-zero on error.
     */
    int (*set_level)(void *ctx, int pin, int level);
    /**
     * Read the actual pad level.
     * @param ctx Implementation handle.
     * @param pin GPIO number.
     * @return 0 or 1.
     */
    int (*get_level)(void *ctx, int pin);
} pin_hal_ops_t;

/** A pin HAL instance: function table plus opaque handle. */
typedef struct {
    const pin_hal_ops_t *ops; /**< Implementation's function table. */
    void *ctx;                /**< Opaque implementation handle. */
} pin_hal_t;

#ifdef __cplusplus
}
#endif
