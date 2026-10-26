/*
 * IGNACC_Controller - composition root.
 *
 * Reads the Kconfig settings, creates the objects, injects dependencies and
 * starts the tasks. No application logic lives here.
 */
#include <assert.h>
#include <stdbool.h>

#include "app_fsm.h"
#include "app_fsm_task.h"
#include "esp_app_desc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "link_if.h"
#include "pin_hal_esp.h"
#include "relay_gpio.h"
#include "sdkconfig.h"
#include "uart_link.h"

/* ---- Compile-time configuration checks --------------------------------- */

#define PIN_IS_FLASH(p)      ((p) >= 6 && (p) <= 11)
#define PIN_IS_INPUT_ONLY(p) ((p) >= 34)
#define PIN_IS_DISCOURAGED(p) \
    ((p) == 0 || (p) == 2 || (p) == 5 || (p) == 12 || (p) == 15 || (p) == 14)

static_assert(!PIN_IS_FLASH(CONFIG_IGNACC_RELAY1_GPIO) && !PIN_IS_INPUT_ONLY(CONFIG_IGNACC_RELAY1_GPIO),
              "IN1 GPIO must be an output-capable, non-flash pin");
static_assert(!PIN_IS_FLASH(CONFIG_IGNACC_RELAY2_GPIO) && !PIN_IS_INPUT_ONLY(CONFIG_IGNACC_RELAY2_GPIO),
              "IN2 GPIO must be an output-capable, non-flash pin");
static_assert(CONFIG_IGNACC_RELAY1_GPIO != CONFIG_IGNACC_RELAY2_GPIO, "IN1 and IN2 must use different GPIOs");
static_assert(CONFIG_IGNACC_RELAY1_GPIO != CONFIG_IGNACC_UART_TX_GPIO &&
              CONFIG_IGNACC_RELAY1_GPIO != CONFIG_IGNACC_UART_RX_GPIO &&
              CONFIG_IGNACC_RELAY2_GPIO != CONFIG_IGNACC_UART_TX_GPIO &&
              CONFIG_IGNACC_RELAY2_GPIO != CONFIG_IGNACC_UART_RX_GPIO,
              "Relay GPIOs must not overlap the UART pins");
static_assert(CONFIG_IGNACC_MAX_LINE_LEN <= LINK_LINE_MAX_LEN, "Max line length exceeds LINK_LINE_MAX_LEN");

#if PIN_IS_DISCOURAGED(CONFIG_IGNACC_RELAY1_GPIO) || PIN_IS_DISCOURAGED(CONFIG_IGNACC_RELAY2_GPIO)
#warning "A relay uses a strapping pin or GPIO14: the relay may click during reset/boot"
#endif

/* ---- Composition --------------------------------------------------------- */

/* Init failed: leave relays in whatever safe state was reached and stop. */
static void halt(void)
{
    for (;;) {
        vTaskDelay(portMAX_DELAY);
    }
}

void app_main(void)
{
    /* 1. Relays first, so both are OFF as early as possible. */
    const pin_hal_t hal = pin_hal_esp();
    const relay_gpio_config_t relay_cfg[PROTO_RELAY_CHANNELS] = {
        { .pin = CONFIG_IGNACC_RELAY1_GPIO, .active_level = CONFIG_IGNACC_RELAY_ACTIVE_LEVEL },
        { .pin = CONFIG_IGNACC_RELAY2_GPIO, .active_level = CONFIG_IGNACC_RELAY_ACTIVE_LEVEL },
    };
    app_fsm_config_t fsm_cfg = {
        .fw_version = esp_app_get_description()->version,
        .settle_ms = CONFIG_IGNACC_RELAY_SETTLE_MS,
        .delay = app_fsm_rtos_delay_ms,
        .delay_ctx = NULL,
    };
    for (unsigned i = 0; i < PROTO_RELAY_CHANNELS; i++) {
        if (relay_gpio_create(&relay_cfg[i], &hal, &fsm_cfg.relays[i]) != RELAY_OK) {
            halt();
        }
    }

    /* 2. Command queue shared by the link (producer) and the FSM (consumer). */
    QueueHandle_t line_queue = xQueueCreate(CONFIG_IGNACC_CMD_QUEUE_DEPTH, sizeof(link_line_t));
    if (line_queue == NULL) {
        halt();
    }

    /* 3. Link, then state machine. Lines received meanwhile wait in the queue. */
    const uart_link_config_t link_cfg = {
        .port = CONFIG_IGNACC_UART_PORT,
        .baud_rate = CONFIG_IGNACC_UART_BAUD,
        .tx_pin = CONFIG_IGNACC_UART_TX_GPIO,
        .rx_pin = CONFIG_IGNACC_UART_RX_GPIO,
        .rx_buf_size = CONFIG_IGNACC_UART_RX_BUF_SIZE,
        .event_queue_len = CONFIG_IGNACC_UART_EVENT_QUEUE_LEN,
        .max_line_len = CONFIG_IGNACC_MAX_LINE_LEN,
        .task_priority = CONFIG_IGNACC_UART_TASK_PRIORITY,
        .task_stack_size = CONFIG_IGNACC_UART_TASK_STACK,
        .line_queue = line_queue,
    };
    if (uart_link_start(&link_cfg, &fsm_cfg.link) != ESP_OK) {
        halt();
    }

    app_fsm_t *fsm = app_fsm_create(&fsm_cfg);
    if (fsm == NULL || !app_fsm_start(fsm)) {
        halt();
    }

    /* 4. Start consuming commands. */
    const app_fsm_task_config_t task_cfg = {
        .line_queue = line_queue,
        .priority = CONFIG_IGNACC_FSM_TASK_PRIORITY,
        .stack_size = CONFIG_IGNACC_FSM_TASK_STACK,
    };
    if (app_fsm_task_start(fsm, &task_cfg) != ESP_OK) {
        halt();
    }
}
