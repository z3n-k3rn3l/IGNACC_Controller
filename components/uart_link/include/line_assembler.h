/**
 * @file line_assembler.h
 * @brief Assembles a byte stream into lines. Pure C.
 *
 * - Lines end with `'\n'`; one trailing `'\r'` is stripped.
 * - Lines longer than `max_len` (excluding terminators) are discarded entirely,
 *   including any command at their end.
 * - Empty lines are not emitted.
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "link_if.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Assembler state. Treat as opaque; it is public only so it can live on the stack. */
typedef struct {
    char buf[LINK_LINE_MAX_LEN + 1]; /**< Line being assembled; +1 for a trailing '\\r'. */
    size_t len;                      /**< Bytes in @ref buf. */
    size_t max_len;                  /**< Longest accepted line. */
    bool discarding;                 /**< Dropping bytes until the next '\\n'. */
} line_asm_t;

/**
 * @brief Initialise an assembler.
 * @param a       The assembler.
 * @param max_len Longest accepted line, clamped to [1, #LINK_LINE_MAX_LEN].
 */
void line_asm_init(line_asm_t *a, size_t max_len);

/**
 * @brief Drop the partial line and everything up to and including the next '\\n'.
 *
 * Call after receive errors or data loss, so a corrupted line is never emitted.
 *
 * @param a The assembler.
 */
void line_asm_discard(line_asm_t *a);

/**
 * @brief Feed one byte.
 * @param a   The assembler.
 * @param c   Received byte.
 * @param out Receives the line when one is complete.
 * @return true if a complete line was written to @p out.
 */
bool line_asm_push(line_asm_t *a, char c, link_line_t *out);

#ifdef __cplusplus
}
#endif
