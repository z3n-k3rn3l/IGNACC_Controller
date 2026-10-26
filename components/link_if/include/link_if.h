/*
 * link_if - transport-independent contracts shared by the link (producer of
 * lines, sink of responses) and the state machine (consumer). Header only,
 * pure C.
 */
#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Upper bound for the configurable maximum line length. */
#define LINK_LINE_MAX_LEN 128

/** One received line, terminator stripped. Not NUL-terminated: use len. */
typedef struct {
    uint16_t len;
    char data[LINK_LINE_MAX_LEN];
} link_line_t;

/** Outgoing side of a link: send a complete response. */
typedef struct {
    /** Send len bytes. Returns 0 on success, non-zero on failure. */
    int (*send)(void *ctx, const char *buf, size_t len);
} link_tx_ops_t;

typedef struct {
    const link_tx_ops_t *ops;
    void *ctx; /*!< Opaque implementation handle */
} link_tx_t;

static inline int link_tx_send(const link_tx_t *tx, const char *buf, size_t len)
{
    return tx->ops->send(tx->ctx, buf, len);
}

#ifdef __cplusplus
}
#endif
