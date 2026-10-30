/**
 * @file link_if.h
 * @brief Transport-independent contracts shared by the link and the state machine.
 *
 * Header only, pure C. The link (uart_link) produces #link_line_t items and
 * implements #link_tx_t; the state machine (app_fsm) consumes lines and sends
 * responses through #link_tx_t. Neither knows about the other.
 */
#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Upper bound for the configurable maximum line length (CONFIG_IGNACC_MAX_LINE_LEN). */
#define LINK_LINE_MAX_LEN 128

/** One received line, terminator stripped. Not NUL-terminated: use @ref len. */
typedef struct {
    uint16_t len;                 /**< Number of valid bytes in @ref data. */
    char data[LINK_LINE_MAX_LEN]; /**< Line bytes. */
} link_line_t;

/** Outgoing side of a link. */
typedef struct {
    /**
     * Send a complete response.
     * @param ctx Implementation handle (link_tx_t::ctx).
     * @param buf Bytes to send.
     * @param len Number of bytes.
     * @return 0 on success, non-zero on failure.
     */
    int (*send)(void *ctx, const char *buf, size_t len);
} link_tx_ops_t;

/** A link's transmit side: function table plus opaque handle. */
typedef struct {
    const link_tx_ops_t *ops; /**< Implementation's function table. */
    void *ctx;                /**< Opaque implementation handle. */
} link_tx_t;

/**
 * @brief Send bytes through a link.
 * @param tx  The link.
 * @param buf Bytes to send.
 * @param len Number of bytes.
 * @return 0 on success, non-zero on failure.
 */
static inline int link_tx_send(const link_tx_t *tx, const char *buf, size_t len)
{
    return tx->ops->send(tx->ctx, buf, len);
}

#ifdef __cplusplus
}
#endif
