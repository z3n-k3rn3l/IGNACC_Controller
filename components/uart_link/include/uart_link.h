/*
 * uart_link - line transport over an ESP-IDF UART.
 *
 * Uses the IDF UART driver (driver-managed ISR, ring buffer and event queue).
 * A receive task assembles bytes into lines and posts each complete line to
 * the injected queue. Knows nothing about relays or the command set.
 */
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "link_if.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int port;
    int baud_rate;
    int tx_pin;
    int rx_pin;
    int rx_buf_size;      /*!< Driver RX ring buffer size (bytes) */
    int event_queue_len;  /*!< Driver event queue length */
    size_t max_line_len;  /*!< 1..LINK_LINE_MAX_LEN */
    UBaseType_t task_priority;
    uint32_t task_stack_size;
    QueueHandle_t line_queue; /*!< Receives link_line_t items; full -> line dropped */
} uart_link_config_t;

/**
 * Install the UART driver (8N1, no flow control), start the receive task and
 * return the transmit side of the link.
 */
esp_err_t uart_link_start(const uart_link_config_t *cfg, link_tx_t *out_tx);

#ifdef __cplusplus
}
#endif
