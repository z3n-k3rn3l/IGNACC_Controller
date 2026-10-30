/**
 * @file app_fsm_task.h
 * @brief FreeRTOS runner for app_fsm.
 *
 * A task blocks on the line queue and feeds each line to the state machine.
 * Kept separate from app_fsm.c so the state machine itself stays pure C.
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

/** Task configuration. */
typedef struct {
    QueueHandle_t line_queue; /**< Delivers #link_line_t items from the link. */
    UBaseType_t priority;     /**< Task priority. */
    uint32_t stack_size;      /**< Task stack size in bytes. */
} app_fsm_task_config_t;

/**
 * @brief Start the FSM task. Only one instance is supported.
 * @param fsm A started (READY) FSM.
 * @param cfg Task configuration.
 * @return ESP_OK; ESP_ERR_INVALID_ARG; ESP_ERR_INVALID_STATE if already
 *         started; ESP_ERR_NO_MEM if the task could not be created.
 */
esp_err_t app_fsm_task_start(app_fsm_t *fsm, const app_fsm_task_config_t *cfg);

/**
 * @brief #app_fsm_delay_fn_t implementation on vTaskDelay.
 *
 * Rounds up, so the delay is never shorter than requested.
 *
 * @param ctx Unused.
 * @param ms  Milliseconds to wait; 0 returns immediately.
 */
void app_fsm_rtos_delay_ms(void *ctx, uint32_t ms);

#ifdef __cplusplus
}
#endif
