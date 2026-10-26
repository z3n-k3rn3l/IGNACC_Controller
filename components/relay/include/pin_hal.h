/*
 * pin_hal - thin digital pin HAL used by relay_gpio, so the driver can be
 * tested on the host with a mock. Pure C.
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    /**
     * Configure pin as input+output (so the level can be read back).
     * Contract: initial_level MUST be latched before the output driver is
     * enabled, so the pin never glitches to the other level.
     * Returns 0 on success.
     */
    int (*configure_output)(void *ctx, int pin, int initial_level);
    /** Drive pin to level (0/1). Returns 0 on success. */
    int (*set_level)(void *ctx, int pin, int level);
    /** Read the actual pad level (0/1). */
    int (*get_level)(void *ctx, int pin);
} pin_hal_ops_t;

typedef struct {
    const pin_hal_ops_t *ops;
    void *ctx;
} pin_hal_t;

#ifdef __cplusplus
}
#endif
