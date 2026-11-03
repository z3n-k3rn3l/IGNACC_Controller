# Code tour

A guided reading order through the firmware, about 45 minutes. It follows the
order in which `app_main()` builds the system, then traces one command from
start to finish. Read each stop's files in the order given; links jump to the
exact line.

Firmware 1.2.0. Line numbers are for `feature/unk-nack`.

---

## Before you start (5 min)

1. Look at the [system context](diagrams/system-context.svg): PC → USB → ESP32 →
   relays → ECU.
2. Look at the [layers](diagrams/layered-architecture.svg): the state machine at
   the top, the hardware at the bottom.
3. Learn these six words:

| Word | Meaning |
|---|---|
| **channel** | One relay: IN1, IN2, … INn. n is set in menuconfig (1–14) |
| **line** | One received text, e.g. `IN1_ON`, without its `\n` |
| **command** | A line the protocol understands |
| **reply** | The answer, e.g. `IN1_ON_OK`, ending with `\r\n` |
| **settle delay** | 15 ms wait after switching a relay, before `_OK` is sent |
| **safe state** | Every relay OFF. Used at boot and after any failure |

---

## Stop 1 — `main`: where everything is built (10 min)

**Read:** [main/main.c](../main/main.c), starting at
[`app_main()`, line 122](../main/main.c#L122).

`app_main()` does five things, in this order. The tour follows the same order:

| Step | Line | What | Tour stop |
|---|---|---|---|
| 1 | [132](../main/main.c#L132) | create one relay per channel, each latched OFF | Stop 2 |
| 2 | [145](../main/main.c#L145) | create the command queue | Stop 3 |
| 3 | [163](../main/main.c#L163) | start the UART link | Stop 3 |
| 4 | [168](../main/main.c#L168) | create and start the state machine | Stop 4 |
| 5 | [179](../main/main.c#L179) | start the state-machine task | Stop 4 |

Then read the top of the file, which runs at **compile time**:
- [line 42](../main/main.c#L42): the 14 possible channel pins as one X-macro list.
- [line 84](../main/main.c#L84): the duplicate-pin check (`popcount`), next to the safe-pin and UART-pin checks.

**Notice:**
- `main` is the only file that reads `sdkconfig`. Everything else receives
  plain structs.
- Any start-up failure calls `halt()`: the relays stay OFF and the board stays
  silent.

**Diagrams:** [module-main](diagrams/module-main.svg),
[kconfig-channel-model](diagrams/kconfig-channel-model.svg),
[activity-boot](diagrams/activity-boot.svg)

---

## Stop 2 — `relay`: switching one pin, safely (10 min)

*From step 1 of `app_main()`.* Read bottom-up: the interface first, then one
implementation, then the group.

1. [relay.h, line 31](../components/relay/include/relay.h#L31): the **interface**.
   A `relay_t` is a function table (`set`, `is_on`) plus an opaque `ctx`.
   [`relay_set()`](../components/relay/include/relay.h#L59) just calls through
   the table.
2. [relay_gpio.c, line 61](../components/relay/relay_gpio.c#L61):
   `relay_gpio_create()`. It writes the **OFF level first**, then makes the pin
   an output, then checks it. That is why a relay never clicks at boot.
3. [relay_gpio.c, line 30](../components/relay/relay_gpio.c#L30):
   `relay_gpio_set()` drives the pin, then reads it back.
4. [pin_hal_esp.c, line 25](../components/relay/pin_hal_esp.c#L25): the only
   place that calls the ESP-IDF GPIO driver.
5. [relay_bank.c, line 31](../components/relay/relay_bank.c#L31):
   `relay_bank_set_all()`, the lower half of `ALL_ON`/`ALL_OFF`. It loops over
   any number of relays and never stops early.

**Notice:** nothing above `relay.h` knows about GPIOs. A relay on an I/O
expander would be a new file implementing `relay_t`, and nothing else would
change.

**Diagrams:** [module-relay](diagrams/module-relay.svg),
[state-relay-channel](diagrams/state-relay-channel.svg)

---

## Stop 3 — `uart_link`: bytes in, lines out (8 min)

*From steps 2–3 of `app_main()`.*

1. [link_if.h](../components/link_if/include/link_if.h): the two shared types.
   `link_line_t` is one received line; `link_tx_t` is "send a reply".
2. [uart_link.c, line 120](../components/uart_link/uart_link.c#L120):
   `uart_link_start()` installs the ESP-IDF UART driver and starts the receive
   task.
3. [uart_link.c, line 90](../components/uart_link/uart_link.c#L90): the receive
   task waits for driver events. Data goes to
   [`uart_link_read_data()`, line 60](../components/uart_link/uart_link.c#L60),
   which feeds bytes to the line assembler and posts complete lines to the queue
   **without waiting**.
4. [line_assembler.c, line 33](../components/uart_link/line_assembler.c#L33):
   `line_asm_push()` collects bytes until `\n`, strips one `\r`, and drops lines
   that are too long.

**Notice:**
- The queue decouples the two tasks: the receiver never waits for a relay to settle.
- This module never looks inside a line.

**Diagrams:** [module-uart-link](diagrams/module-uart-link.svg),
[state-line-assembler](diagrams/state-line-assembler.svg)

---

## Stop 4 — `app_fsm`: the decision maker (10 min)

*From steps 4–5 of `app_main()`.* This is the heart of the firmware.

1. [app_fsm.c, line 163](../components/app_fsm/app_fsm.c#L163):
   `app_fsm_create()` copies the relay handles, so the FSM is their **sole
   owner**.
2. [app_fsm.c, line 191](../components/app_fsm/app_fsm.c#L191): `app_fsm_start()`
   drives every relay OFF, then the state becomes READY.
3. [app_fsm_task.c, line 28](../components/app_fsm/app_fsm_task.c#L28): the task
   loop takes a line from the queue and calls `app_fsm_handle_line()`.
4. [app_fsm.c, line 198](../components/app_fsm/app_fsm.c#L198):
   `app_fsm_handle_line()` parses the line (Stop 5), then
   [`handle_command()`, line 134](../components/app_fsm/app_fsm.c#L134)
   dispatches it:
   - `IN<n>` → [`handle_relay_command()`, line 71](../components/app_fsm/app_fsm.c#L71):
     drive, settle, verify, reply.
   - `ALL_*` → [`handle_all_command()`, line 93](../components/app_fsm/app_fsm.c#L93):
     drive all, settle **once**, verify all, reply.
   - anything else → `UNK_NACK`.
5. Any failure → [`drive_safe_state()`, line 46](../components/app_fsm/app_fsm.c#L46)
   switches every relay OFF, then the reply is `_FAIL`.

**Notice:**
- One line is handled completely before the next, so commands never overlap.
- `app_fsm.c` has no ESP-IDF calls. The delay function is injected, so the same
  code runs in the unit tests.

**Diagrams:** [state-app-fsm](diagrams/state-app-fsm.svg),
[activity-command-handling](diagrams/activity-command-handling.svg),
[module-app-fsm](diagrams/module-app-fsm.svg)

---

## Stop 5 — `protocol`: text to command, result to text (5 min)

*Called by `app_fsm` in Stop 4.*

1. [protocol.h](../components/protocol/include/protocol.h): the grammar table at
   the top lists every command and reply.
2. [protocol.c, line 80](../components/protocol/protocol.c#L80): `proto_parse()`
   matches fixed commands, then
   [`parse_relay()`, line 55](../components/protocol/protocol.c#L55) handles
   `IN<n>_ON/OFF` (n = 1–16, no leading zeros).
3. [protocol.c, line 107](../components/protocol/protocol.c#L107) onward:
   `proto_format_result()`, `_status()` and `_nack()` build the replies.

**Notice:** the protocol accepts `IN1`–`IN16` but doesn't know the board.
`app_fsm` decides that `IN3` doesn't exist on a 2-channel board and answers
`UNK_NACK`.

**Diagram:** [module-protocol](diagrams/module-protocol.svg)

---

## Recap: follow `IN1_ON` from end to end

Every hop you just read, in the order a real command takes them (the
[sequence diagram](diagrams/sequence-relay-command.svg) shows the same thing):

1. The PC sends `IN1_ON\n`. The ESP-IDF UART driver raises a data event.
2. [`uart_link_task()`](../components/uart_link/uart_link.c#L90) →
   [`uart_link_read_data()`](../components/uart_link/uart_link.c#L60) →
   [`line_asm_push()`](../components/uart_link/line_assembler.c#L33) → the line is
   posted to the queue.
3. [`app_fsm_task()`](../components/app_fsm/app_fsm_task.c#L28) takes it →
   [`app_fsm_handle_line()`](../components/app_fsm/app_fsm.c#L198) →
   [`proto_parse()`](../components/protocol/protocol.c#L80) gives "channel 1, ON".
4. [`handle_command()`](../components/app_fsm/app_fsm.c#L134) →
   [`handle_relay_command()`](../components/app_fsm/app_fsm.c#L71) →
   [`relay_bank_set()`](../components/relay/relay_bank.c#L15) →
   [`relay_gpio_set()`](../components/relay/relay_gpio.c#L30). GPIO26 goes LOW and
   is read back.
5. A 15 ms settle wait, then a read-back check. `proto_format_result()` makes
   `IN1_ON_OK`.
6. [`uart_link_send()`](../components/uart_link/uart_link.c#L39) writes
   `IN1_ON_OK\r\n` back to the PC, about 19 ms after it was sent.

---

## After the tour

| Next | Why |
|---|---|
| [test/host/test_app_fsm.c](../test/host/test_app_fsm.c) | The best executable description of the behaviour: each test name is a rule |
| [configuration-and-architecture.md](configuration-and-architecture.md) | Every setting, and how the Kconfig channel model works |
| [diagrams/README.md](diagrams/README.md) | All 20 diagrams, each explained in plain words |
| [tools/ignacc_tester](../tools/ignacc_tester) | How the board is tested from the PC |
| Doxygen (`doxygen` in the project root) | API pages with every function, struct and diagram |
