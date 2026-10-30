/**
 * @file relay_bank.h
 * @brief A group of relays addressed by index; the layer behind ALL_ON / ALL_OFF.
 *
 * Pure C. Works on the abstract #relay_t interface only, so one bank can hold
 * any number of relays of any implementation (GPIO today, an I/O expander or
 * shift register later). The state machine asks the bank to "set all" and
 * never needs to know how many relays exist or how each one switches.
 *
 * The bank does not own the relays; it only references an array of them.
 */
#pragma once

#include <stdbool.h>

#include "relay.h"

#ifdef __cplusplus
extern "C" {
#endif

/** A bank of relays. Index 0 is channel IN1. */
typedef struct {
    const relay_t *relays; /**< Array of @ref count relays (not owned). */
    unsigned count;        /**< Number of relays in the bank. */
} relay_bank_t;

/**
 * @brief Number of relays in a bank.
 * @param bank The bank.
 * @return relay_bank_t::count.
 */
static inline unsigned relay_bank_count(const relay_bank_t *bank)
{
    return bank->count;
}

/**
 * @brief Drive one relay.
 * @param bank  The bank.
 * @param index Relay index, 0..count-1 (index 0 = IN1).
 * @param on    true = ON, false = OFF.
 * @return The relay's result, or #RELAY_ERR_ARG if @p bank is NULL or
 *         @p index is out of range (no relay is touched then).
 */
relay_err_t relay_bank_set(const relay_bank_t *bank, unsigned index, bool on);

/**
 * @brief Read one relay back from the hardware.
 * @param bank  The bank.
 * @param index Relay index, 0..count-1.
 * @return true if that relay reads as ON; false if it reads OFF, @p bank is
 *         NULL or @p index is out of range.
 */
bool relay_bank_is_on(const relay_bank_t *bank, unsigned index);

/**
 * @brief Drive every relay to the same state, back to back.
 *
 * Every relay is attempted even if an earlier one fails, so one faulty
 * channel can never leave the others in the wrong state. The relays switch
 * microseconds apart; the caller waits one settle delay afterwards.
 *
 * @param bank The bank.
 * @param on   true = all ON, false = all OFF.
 * @return #RELAY_OK if every relay succeeded, otherwise the first error seen
 *         (#RELAY_ERR_ARG if @p bank is NULL).
 */
relay_err_t relay_bank_set_all(const relay_bank_t *bank, bool on);

/**
 * @brief Check that every relay reads back in the given state.
 * @param bank The bank.
 * @param on   State to check for.
 * @return true if every relay reads as @p on; false otherwise or if @p bank is NULL.
 */
bool relay_bank_all_are(const relay_bank_t *bank, bool on);

#ifdef __cplusplus
}
#endif
