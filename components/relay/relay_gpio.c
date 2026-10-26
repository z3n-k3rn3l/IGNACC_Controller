#include "relay_gpio.h"

#include <stdlib.h>

typedef struct {
    pin_hal_t hal;
    int pin;
    int on_level;
    int off_level;
} relay_gpio_t;

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

static bool relay_gpio_is_on(void *ctx)
{
    relay_gpio_t *self = ctx;
    return self->hal.ops->get_level(self->hal.ctx, self->pin) == self->on_level;
}

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
