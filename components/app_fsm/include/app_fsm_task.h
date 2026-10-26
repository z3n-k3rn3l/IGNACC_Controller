/*
 * app_fsm_task - FreeRTOS runner for app_fsm: blocks on the line queue and
 * feeds each line to the state machine.
 */
#pragma once

#include <stdint.h>

#include "app_fsm.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    QueueHandle_t line_queue; /*!< Delivers link_line_t items */
    UBaseType_t priority;
    uint32_t stack_size;
} app_fsm_task_config_t;

/** Start the task. fsm must already be started (READY). */
esp_err_t app_fsm_task_start(app_fsm_t *fsm, const app_fsm_task_config_t *cfg);

/**
 * app_fsm_delay_fn_t implementation on vTaskDelay. Rounds up so the delay is
 * never shorter than requested. ctx is unused.
 */
void app_fsm_rtos_delay_ms(void *ctx, uint32_t ms);

#ifdef __cplusplus
}
#endif
