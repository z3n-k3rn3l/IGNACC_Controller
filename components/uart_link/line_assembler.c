#include "line_assembler.h"

#include <string.h>

void line_asm_init(line_asm_t *a, size_t max_len)
{
    if (max_len < 1) {
        max_len = 1;
    } else if (max_len > LINK_LINE_MAX_LEN) {
        max_len = LINK_LINE_MAX_LEN;
    }
    a->len = 0;
    a->max_len = max_len;
    a->discarding = false;
}

void line_asm_discard(line_asm_t *a)
{
    a->len = 0;
    a->discarding = true;
}

bool line_asm_push(line_asm_t *a, char c, link_line_t *out)
{
    if (c == '\n') {
        size_t n = a->len;
        bool was_discarding = a->discarding;

        a->len = 0;
        a->discarding = false;
        if (was_discarding) {
            return false;
        }
        if (n > 0 && a->buf[n - 1] == '\r') {
            n--;
        }
        if (n == 0 || n > a->max_len) {
            return false;
        }
        memcpy(out->data, a->buf, n);
        out->len = (uint16_t)n;
        return true;
    }

    if (a->discarding) {
        return false;
    }
    if (a->len == a->max_len + 1) {
        /* Too long even allowing for a trailing '\r': drop the whole line. */
        line_asm_discard(a);
        return false;
    }
    a->buf[a->len++] = c;
    return false;
}
