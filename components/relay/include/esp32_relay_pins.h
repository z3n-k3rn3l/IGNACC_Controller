/*
 * esp32_relay_pins - which ESP32 (WROOM-32) GPIOs are safe to drive a relay.
 * Pure preprocessor macros, usable in #if, static_assert and host tests.
 *
 * Excluded:
 *   0, 2, 5, 12, 15  strapping pins (sampled at reset; a relay could click or
 *                    the chip could boot into the wrong mode)
 *   1, 3             UART0 TX/RX (the protocol link)
 *   6..11            SPI flash
 *   14               outputs a PWM signal during boot
 *   34..39           input only
 *
 * Safe: 4, 13, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33  (14 pins)
 * (16/17 are used by PSRAM on WROVER modules; this firmware targets WROOM-32.)
 */
#pragma once

#define ESP32_RELAY_PIN_IS_SAFE(p) \
    ((p) == 4 || (p) == 13 || (p) == 16 || (p) == 17 || (p) == 18 || (p) == 19 || \
     (p) == 21 || (p) == 22 || (p) == 23 || (p) == 25 || (p) == 26 || (p) == 27 || \
     (p) == 32 || (p) == 33)

/** Number of safe relay pins = the maximum channel count on this module. */
#define ESP32_RELAY_PIN_SAFE_COUNT 14
