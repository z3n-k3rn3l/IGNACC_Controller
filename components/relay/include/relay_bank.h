/*
 * relay_bank - a group of relays addressed by index. Pure C.
 *
 * Works on the abstract relay_t interface only, so a bank can mix any relay
 * implementations (GPIO today, an I/O expander or shift register later) and
 * any count, without changes here or in its users.
 */
#pragma once

#include <stdbool.h>

#include "relay.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const relay_t *relays; /*!< Array of count relays; index 0 is channel IN1 */
    unsigned count;
} relay_bank_t;

static inline unsigned relay_bank_count(const relay_bank_t *bank)
{
    return bank->count;
}

/** Drive one relay. RELAY_ERR_ARG if index >= count. */
relay_err_t relay_bank_set(const relay_bank_t *bank, unsigned index, bool on);

/** Read back one relay. false if index >= count. */
bool relay_bank_is_on(const relay_bank_t *bank, unsigned index);

/**
 * Drive every relay to the same state, back to back. Every relay is attempted
 * even if an earlier one fails.
 * @return RELAY_OK if all succeeded, otherwise the first error seen.
 */
relay_err_t relay_bank_set_all(const relay_bank_t *bank, bool on);

/** true if every relay reads back as on (or every relay as off). */
bool relay_bank_all_are(const relay_bank_t *bank, bool on);

#ifdef __cplusplus
}
#endif
