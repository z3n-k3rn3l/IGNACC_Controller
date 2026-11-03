/**
 * @file relay_gpio.c
 * @brief GPIO relay: one #relay_t per pin, verified by read-back.
 *
 * Tour stop 2. relay_gpio_create() is where the "no ON glitch at boot"
 * guarantee lives: the OFF level is written before the pin becomes an output.
 * All pin access goes through the injected #pin_hal_t.
 *
 * @see docs/diagrams/module-relay.svg
 */
#include "relay_gpio.h"

#include <stdlib.h>

/** Private state of one GPIO relay (the opaque relay_t::ctx). */
typedef struct {
    pin_hal_t hal;  /**< Pin HAL (copied at create). */
    int pin;        /**< GPIO number. */
    int on_level;   /**< Pin level for ON (the configured active level). */
    int off_level;  /**< Pin level for OFF (the opposite level). */
} relay_gpio_t;

/**
 * @brief relay_ops_t::set for a GPIO relay: drive the pin, then read it back.
 * @param ctx The relay_gpio_t.
 * @param on  true = ON, false = OFF.
 * @return #RELAY_OK, #RELAY_ERR_IO if the HAL refused, #RELAY_ERR_VERIFY if the
 *         pin doesn't read back at the commanded level.
 */
static relay_err_t relay_gpio_set(void *ctx, bool on)
{
    relay_gpio_t *self = ctx;
    int level = on ? self->on_level : self->off_level;

    if (self->hal.ops->set_level(self->hal.ctx, self->pin, level) != 0) {
        return RELAY_ERR_IO;
    }
    if (self->hal.ops->get_level(self->hal.ctx, self->pin) != level) {
        return RELAY_ERR_VERIFY;
    }
    return RELAY_OK;
}

/**
 * @brief relay_ops_t::is_on for a GPIO relay: read the actual pad level.
 * @param ctx The relay_gpio_t.
 * @return true if the pin is at the ON level.
 */
static bool relay_gpio_is_on(void *ctx)
{
    relay_gpio_t *self = ctx;
    return self->hal.ops->get_level(self->hal.ctx, self->pin) == self->on_level;
}

/** Function table shared by every GPIO relay. */
static const relay_ops_t s_relay_gpio_ops = {
    .set = relay_gpio_set,
    .is_on = relay_gpio_is_on,
};

relay_err_t relay_gpio_create(const relay_gpio_config_t *cfg, const pin_hal_t *hal, relay_t *out)
{
    if (cfg == NULL || hal == NULL || hal->ops == NULL || out == NULL || cfg->pin < 0 ||
        (cfg->active_level != 0 && cfg->active_level != 1)) {
        return RELAY_ERR_ARG;
    }

    relay_gpio_t *self = malloc(sizeof(*self));
    if (self == NULL) {
        return RELAY_ERR_NOMEM;
    }
    self->hal = *hal;
    self->pin = cfg->pin;
    self->on_level = cfg->active_level;
    self->off_level = !cfg->active_level;

    /* Latch OFF before the output driver is enabled: no ON glitch at boot. */
    if (self->hal.ops->configure_output(self->hal.ctx, self->pin, self->off_level) != 0) {
        free(self);
        return RELAY_ERR_IO;
    }
    if (self->hal.ops->get_level(self->hal.ctx, self->pin) != self->off_level) {
        free(self);
        return RELAY_ERR_VERIFY;
    }

    out->ops = &s_relay_gpio_ops;
    out->ctx = self;
    return RELAY_OK;
}

void relay_gpio_destroy(relay_t *relay)
{
    if (relay == NULL) {
        return;
    }
    free(relay->ctx);
    relay->ops = NULL;
    relay->ctx = NULL;
}
