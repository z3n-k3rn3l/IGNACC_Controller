#include "relay_bank.h"

#include <stddef.h>

relay_err_t relay_bank_set(const relay_bank_t *bank, unsigned index, bool on)
{
    if (bank == NULL || index >= bank->count) {
        return RELAY_ERR_ARG;
    }
    return relay_set(&bank->relays[index], on);
}

bool relay_bank_is_on(const relay_bank_t *bank, unsigned index)
{
    if (bank == NULL || index >= bank->count) {
        return false;
    }
    return relay_is_on(&bank->relays[index]);
}

relay_err_t relay_bank_set_all(const relay_bank_t *bank, bool on)
{
    if (bank == NULL) {
        return RELAY_ERR_ARG;
    }
    relay_err_t first = RELAY_OK;
    for (unsigned i = 0; i < bank->count; i++) {
        relay_err_t err = relay_set(&bank->relays[i], on);
        if (err != RELAY_OK && first == RELAY_OK) {
            first = err;
        }
    }
    return first;
}

bool relay_bank_all_are(const relay_bank_t *bank, bool on)
{
    if (bank == NULL) {
        return false;
    }
    for (unsigned i = 0; i < bank->count; i++) {
        if (relay_is_on(&bank->relays[i]) != on) {
            return false;
        }
    }
    return true;
}
