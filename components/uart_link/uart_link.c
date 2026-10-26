#include "uart_link.h"

#include <stdlib.h>

#include "driver/uart.h"
#include "freertos/task.h"
#include "line_assembler.h"

#define RX_CHUNK_SIZE 64

typedef struct {
    uart_port_t port;
    QueueHandle_t events;
    QueueHandle_t lines;
    line_asm_t assembler;
} uart_link_t;

static int uart_link_send(void *ctx, const char *buf, size_t len)
{
    uart_link_t *self = ctx;
    /* No TX ring buffer: blocks until all bytes are in the hardware FIFO. */
    return uart_write_bytes(self->port, buf, len) == (int)len ? 0 : -1;
}

static const link_tx_ops_t s_uart_link_tx_ops = {
    .send = uart_link_send,
};

static void uart_link_read_data(uart_link_t *self, size_t available)
{
    uint8_t chunk[RX_CHUNK_SIZE];
    link_line_t line;

    while (available > 0) {
        size_t want = available < sizeof(chunk) ? available : sizeof(chunk);
        int n = uart_read_bytes(self->port, chunk, want, 0);
        if (n <= 0) {
            return;
        }
        available -= (size_t)n;
        for (int i = 0; i < n; i++) {
            if (line_asm_push(&self->assembler, (char)chunk[i], &line)) {
                /* Never block the receiver: if the queue is full the line is dropped. */
                (void)xQueueSend(self->lines, &line, 0);
            }
        }
    }
}

static void uart_link_task(void *arg)
{
    uart_link_t *self = arg;
    uart_event_t event;

    for (;;) {
        if (xQueueReceive(self->events, &event, portMAX_DELAY) != pdTRUE) {
            continue;
        }
        switch (event.type) {
        case UART_DATA:
            uart_link_read_data(self, event.size);
            break;
        case UART_FIFO_OVF:
        case UART_BUFFER_FULL:
            /* Bytes were lost: drop everything buffered and the line in progress. */
            uart_flush_input(self->port);
            xQueueReset(self->events);
            line_asm_discard(&self->assembler);
            break;
        case UART_FRAME_ERR:
        case UART_PARITY_ERR:
            line_asm_discard(&self->assembler);
            break;
        default:
            break;
        }
    }
}

esp_err_t uart_link_start(const uart_link_config_t *cfg, link_tx_t *out_tx)
{
    if (cfg == NULL || out_tx == NULL || cfg->line_queue == NULL || cfg->max_line_len < 1 ||
        cfg->max_line_len > LINK_LINE_MAX_LEN) {
        return ESP_ERR_INVALID_ARG;
    }

    uart_link_t *self = calloc(1, sizeof(*self));
    if (self == NULL) {
        return ESP_ERR_NO_MEM;
    }
    self->port = (uart_port_t)cfg->port;
    self->lines = cfg->line_queue;
    line_asm_init(&self->assembler, cfg->max_line_len);

    const uart_config_t uart_cfg = {
        .baud_rate = cfg->baud_rate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    esp_err_t err = uart_driver_install(self->port, cfg->rx_buf_size, 0, cfg->event_queue_len,
                                        &self->events, 0);
    if (err != ESP_OK) {
        free(self);
        return err;
    }
    err = uart_param_config(self->port, &uart_cfg);
    if (err == ESP_OK) {
        err = uart_set_pin(self->port, cfg->tx_pin, cfg->rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    }
    if (err == ESP_OK) {
        err = uart_flush_input(self->port);
    }
    if (err == ESP_OK &&
        xTaskCreate(uart_link_task, "uart_link", cfg->task_stack_size, self, cfg->task_priority, NULL) != pdPASS) {
        err = ESP_ERR_NO_MEM;
    }
    if (err != ESP_OK) {
        uart_driver_delete(self->port);
        free(self);
        return err;
    }

    out_tx->ops = &s_uart_link_tx_ops;
    out_tx->ctx = self;
    return ESP_OK;
}
