/*
 * pin_hal_esp - pin_hal_t implementation on top of the ESP-IDF GPIO driver.
 */
#pragma once

#include "pin_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/** The ESP-IDF GPIO HAL instance (stateless; ctx is unused). */
pin_hal_t pin_hal_esp(void);

#ifdef __cplusplus
}
#endif
