/*
 * line_assembler - assembles a byte stream into lines. Pure C.
 *
 * - Lines end with '\n'; one trailing '\r' is stripped.
 * - Lines longer than max_len (excluding terminators) are discarded entirely.
 * - Empty lines are not emitted.
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "link_if.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char buf[LINK_LINE_MAX_LEN + 1]; /*!< +1: room for a trailing '\r' */
    size_t len;
    size_t max_len;
    bool discarding; /*!< Dropping bytes until the next '\n' */
} line_asm_t;

/** Initialise. max_len is clamped to [1, LINK_LINE_MAX_LEN]. */
void line_asm_init(line_asm_t *a, size_t max_len);

/**
 * Drop the partial line and everything up to and including the next '\n'.
 * Use after receive errors or data loss, so a corrupted line is never emitted.
 */
void line_asm_discard(line_asm_t *a);

/**
 * Feed one byte.
 * @return true if a complete line was written to *out.
 */
bool line_asm_push(line_asm_t *a, char c, link_line_t *out);

#ifdef __cplusplus
}
#endif
