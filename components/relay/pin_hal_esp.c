#include "pin_hal_esp.h"

#include "driver/gpio.h"

static int esp_configure_output(void *ctx, int pin, int initial_level)
{
    (void)ctx;
    /* Writing the output register before enabling the driver latches the
     * level, so the pin comes up at initial_level with no glitch. */
    if (gpio_set_level((gpio_num_t)pin, (uint32_t)initial_level) != ESP_OK) {
        return -1;
    }
    const gpio_config_t io = {
        .pin_bit_mask = 1ULL << pin,
        .mode = GPIO_MODE_INPUT_OUTPUT, /* input enabled for read-back */
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    return gpio_config(&io) == ESP_OK ? 0 : -1;
}

static int esp_set_level(void *ctx, int pin, int level)
{
    (void)ctx;
    return gpio_set_level((gpio_num_t)pin, (uint32_t)level) == ESP_OK ? 0 : -1;
}

static int esp_get_level(void *ctx, int pin)
{
    (void)ctx;
    return gpio_get_level((gpio_num_t)pin);
}

static const pin_hal_ops_t s_esp_ops = {
    .configure_output = esp_configure_output,
    .set_level = esp_set_level,
    .get_level = esp_get_level,
};

pin_hal_t pin_hal_esp(void)
{
    return (pin_hal_t){ .ops = &s_esp_ops, .ctx = NULL };
}
