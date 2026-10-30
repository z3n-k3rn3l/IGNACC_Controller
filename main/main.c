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
#include "esp32_relay_pins.h"
#include "esp_app_desc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "link_if.h"
#include "pin_hal_esp.h"
#include "relay_bank.h"
#include "relay_gpio.h"
#include "sdkconfig.h"
#include "uart_link.h"

/* ---- Relay channel table (from Kconfig) --------------------------------- */

/* X(channel, gpio) for every possible channel. Only the first
 * CONFIG_IGNACC_RELAY_COUNT are used; the rest keep their Kconfig defaults. */
#define IGNACC_RELAY_GPIO_LIST(X)                                               \
    X(1, CONFIG_IGNACC_RELAY1_GPIO)   X(2, CONFIG_IGNACC_RELAY2_GPIO)           \
    X(3, CONFIG_IGNACC_RELAY3_GPIO)   X(4, CONFIG_IGNACC_RELAY4_GPIO)           \
    X(5, CONFIG_IGNACC_RELAY5_GPIO)   X(6, CONFIG_IGNACC_RELAY6_GPIO)           \
    X(7, CONFIG_IGNACC_RELAY7_GPIO)   X(8, CONFIG_IGNACC_RELAY8_GPIO)           \
    X(9, CONFIG_IGNACC_RELAY9_GPIO)   X(10, CONFIG_IGNACC_RELAY10_GPIO)         \
    X(11, CONFIG_IGNACC_RELAY11_GPIO) X(12, CONFIG_IGNACC_RELAY12_GPIO)         \
    X(13, CONFIG_IGNACC_RELAY13_GPIO) X(14, CONFIG_IGNACC_RELAY14_GPIO)

#define RELAY_GPIO_ENTRY(ch, gpio) gpio,
static const int s_relay_gpios[] = { IGNACC_RELAY_GPIO_LIST(RELAY_GPIO_ENTRY) };

/* ---- Compile-time configuration checks --------------------------------- */

#define RELAY_USED(ch) ((ch) <= CONFIG_IGNACC_RELAY_COUNT)

#define RELAY_PIN_CHECK(ch, gpio)                                                       \
    static_assert(!RELAY_USED(ch) || ESP32_RELAY_PIN_IS_SAFE(gpio),                     \
                  "IN" #ch " GPIO is not a safe relay pin (see esp32_relay_pins.h)");   \
    static_assert(!RELAY_USED(ch) ||                                                    \
                  ((gpio) != CONFIG_IGNACC_UART_TX_GPIO && (gpio) != CONFIG_IGNACC_UART_RX_GPIO), \
                  "IN" #ch " GPIO overlaps the UART pins");
IGNACC_RELAY_GPIO_LIST(RELAY_PIN_CHECK)

/* One bit per used GPIO: fewer bits than channels means two channels share a pin. */
#define RELAY_PIN_BIT(ch, gpio) | (RELAY_USED(ch) ? (1ULL << (gpio)) : 0ULL)
#define RELAY_PIN_MASK (0ULL IGNACC_RELAY_GPIO_LIST(RELAY_PIN_BIT))
static_assert(__builtin_popcountll(RELAY_PIN_MASK) == CONFIG_IGNACC_RELAY_COUNT,
              "Two relay channels use the same GPIO");

static_assert(CONFIG_IGNACC_RELAY_COUNT <= ESP32_RELAY_PIN_SAFE_COUNT, "More channels than safe GPIOs");
static_assert(CONFIG_IGNACC_RELAY_COUNT <= PROTO_MAX_CHANNELS, "More channels than the protocol supports");
static_assert(sizeof(s_relay_gpios) / sizeof(s_relay_gpios[0]) >= CONFIG_IGNACC_RELAY_COUNT,
              "Relay GPIO table shorter than the channel count");
static_assert(CONFIG_IGNACC_MAX_LINE_LEN <= LINK_LINE_MAX_LEN, "Max line length exceeds LINK_LINE_MAX_LEN");

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
    /* 1. Relays first, so all are OFF as early as possible. */
    const pin_hal_t hal = pin_hal_esp();
    relay_t relays[CONFIG_IGNACC_RELAY_COUNT];
    for (unsigned i = 0; i < CONFIG_IGNACC_RELAY_COUNT; i++) {
        const relay_gpio_config_t relay_cfg = {
            .pin = s_relay_gpios[i],
            .active_level = CONFIG_IGNACC_RELAY_ACTIVE_LEVEL,
        };
        if (relay_gpio_create(&relay_cfg, &hal, &relays[i]) != RELAY_OK) {
            halt();
        }
    }
    app_fsm_config_t fsm_cfg = {
        .relays = { .relays = relays, .count = CONFIG_IGNACC_RELAY_COUNT },
        .fw_version = esp_app_get_description()->version,
        .settle_ms = CONFIG_IGNACC_RELAY_SETTLE_MS,
        .delay = app_fsm_rtos_delay_ms,
        .delay_ctx = NULL,
    };

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

    /* The FSM copies the relay handles, so the local array may go out of scope. */
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
