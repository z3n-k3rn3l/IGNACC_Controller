/**
 * @file uart_link.h
 * @brief Line transport over an ESP-IDF UART.
 *
 * Uses the IDF UART driver (driver-managed ISR, ring buffer and event queue;
 * no custom ISR). A receive task assembles bytes into lines with
 * line_assembler and posts each complete line to the injected queue. Knows
 * nothing about relays or the command set.
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

/** UART link configuration. */
typedef struct {
    int port;                   /**< UART port number (0 = USB bridge on the devkit). */
    int baud_rate;              /**< Baud rate; frames are always 8N1. */
    int tx_pin;                 /**< TX GPIO. */
    int rx_pin;                 /**< RX GPIO. */
    int rx_buf_size;            /**< Driver RX ring buffer size in bytes (> 128). */
    int event_queue_len;        /**< Driver event queue length. */
    size_t max_line_len;        /**< Longest accepted line, 1..#LINK_LINE_MAX_LEN. */
    UBaseType_t task_priority;  /**< Receive task priority. */
    uint32_t task_stack_size;   /**< Receive task stack size in bytes. */
    QueueHandle_t line_queue;   /**< Receives #link_line_t items. If full, new lines are dropped. */
} uart_link_config_t;

/**
 * @brief Install the UART driver, start the receive task, return the transmit side.
 *
 * Frames are 8N1 without flow control. Pending RX bytes are flushed at start.
 *
 * @param cfg    Configuration.
 * @param out_tx Receives the link's transmit side on success.
 * @return ESP_OK; ESP_ERR_INVALID_ARG; ESP_ERR_NO_MEM; or a UART driver error.
 */
esp_err_t uart_link_start(const uart_link_config_t *cfg, link_tx_t *out_tx);

#ifdef __cplusplus
}
#endif
