#include "app_fsm_task.h"

#include "freertos/task.h"
#include "link_if.h"

typedef struct {
    app_fsm_t *fsm;
    QueueHandle_t lines;
} fsm_task_args_t;

static fsm_task_args_t s_args;

static void app_fsm_task(void *arg)
{
    fsm_task_args_t *args = arg;
    link_line_t line;

    for (;;) {
        if (xQueueReceive(args->lines, &line, portMAX_DELAY) == pdTRUE) {
            app_fsm_handle_line(args->fsm, line.data, line.len);
        }
    }
}

esp_err_t app_fsm_task_start(app_fsm_t *fsm, const app_fsm_task_config_t *cfg)
{
    if (fsm == NULL || cfg == NULL || cfg->line_queue == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_args.fsm != NULL) {
        return ESP_ERR_INVALID_STATE; /* single instance */
    }
    s_args.fsm = fsm;
    s_args.lines = cfg->line_queue;
    if (xTaskCreate(app_fsm_task, "app_fsm", cfg->stack_size, &s_args, cfg->priority, NULL) != pdPASS) {
        s_args.fsm = NULL;
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

void app_fsm_rtos_delay_ms(void *ctx, uint32_t ms)
{
    (void)ctx;
    if (ms == 0) {
        return;
    }
    /* +1 tick: vTaskDelay(n) can return up to one tick early. */
    TickType_t ticks = (TickType_t)((ms + portTICK_PERIOD_MS - 1) / portTICK_PERIOD_MS) + 1;
    vTaskDelay(ticks);
}
