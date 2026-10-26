#include <string.h>

#include "app_fsm.h"
#include "unity.h"

/* ---- Event log shared by the mocks (checks call ordering) -------------- */

typedef enum { EV_SET, EV_DELAY, EV_SEND } event_kind_t;

typedef struct {
    event_kind_t kind;
    int a;
    int b;
} event_t;

static event_t s_events[64];
static int s_event_count;

static void log_event(event_kind_t kind, int a, int b)
{
    TEST_ASSERT_LESS_THAN_INT(64, s_event_count);
    s_events[s_event_count++] = (event_t){ kind, a, b };
}

/* ---- Mock relay --------------------------------------------------------- */

typedef struct {
    int id;
    bool on;
    relay_err_t fail_with;   /* returned by set when != RELAY_OK */
    bool drops_after_settle; /* reads OFF after the settle delay */
    int set_calls;
} mock_relay_t;

static mock_relay_t s_relay[2];

static relay_err_t mock_relay_set(void *ctx, bool on)
{
    mock_relay_t *r = ctx;
    r->set_calls++;
    log_event(EV_SET, r->id, on);
    if (r->fail_with != RELAY_OK) {
        return r->fail_with;
    }
    r->on = on;
    return RELAY_OK;
}

static bool mock_relay_is_on(void *ctx)
{
    mock_relay_t *r = ctx;
    return r->on;
}

static const relay_ops_t s_relay_ops = { .set = mock_relay_set, .is_on = mock_relay_is_on };

/* ---- Mock link ---------------------------------------------------------- */

static char s_out[512];
static size_t s_out_len;

static int mock_send(void *ctx, const char *buf, size_t len)
{
    (void)ctx;
    TEST_ASSERT_LESS_OR_EQUAL_size_t(sizeof(s_out) - 1, s_out_len + len);
    memcpy(s_out + s_out_len, buf, len);
    s_out_len += len;
    s_out[s_out_len] = '\0';
    log_event(EV_SEND, (int)len, 0);
    return 0;
}

static const link_tx_ops_t s_link_ops = { .send = mock_send };

/* ---- Fake delay --------------------------------------------------------- */

static void fake_delay(void *ctx, uint32_t ms)
{
    (void)ctx;
    log_event(EV_DELAY, (int)ms, 0);
    for (int i = 0; i < 2; i++) {
        if (s_relay[i].drops_after_settle) {
            s_relay[i].on = false;
        }
    }
}

/* ---- Fixture ------------------------------------------------------------ */

static app_fsm_t *s_fsm;

static app_fsm_config_t make_config(uint32_t settle_ms)
{
    app_fsm_config_t cfg = {
        .relays = {
            { .ops = &s_relay_ops, .ctx = &s_relay[0] },
            { .ops = &s_relay_ops, .ctx = &s_relay[1] },
        },
        .link = { .ops = &s_link_ops, .ctx = NULL },
        .fw_version = "1.2.3",
        .settle_ms = settle_ms,
        .delay = fake_delay,
        .delay_ctx = NULL,
    };
    return cfg;
}

static void clear_logs(void)
{
    s_event_count = 0;
    s_out_len = 0;
    s_out[0] = '\0';
    s_relay[0].set_calls = 0;
    s_relay[1].set_calls = 0;
}

void setUp(void)
{
    memset(s_relay, 0, sizeof(s_relay));
    s_relay[0].id = 0;
    s_relay[1].id = 1;
    clear_logs();
    const app_fsm_config_t cfg = make_config(15);
    s_fsm = app_fsm_create(&cfg);
    TEST_ASSERT_NOT_NULL(s_fsm);
}

void tearDown(void)
{
    app_fsm_destroy(s_fsm);
    s_fsm = NULL;
}

static void line(const char *s)
{
    app_fsm_handle_line(s_fsm, s, strlen(s));
}

static void start_ready(void)
{
    TEST_ASSERT_TRUE(app_fsm_start(s_fsm));
    clear_logs();
}

/* ---- Tests -------------------------------------------------------------- */

static void test_init_state_ignores_lines(void)
{
    TEST_ASSERT_EQUAL(APP_FSM_STATE_INIT, app_fsm_state(s_fsm));
    line("HeyU");
    line("IN1_ON");
    TEST_ASSERT_EQUAL_size_t(0, s_out_len);
    TEST_ASSERT_EQUAL_INT(0, s_relay[0].set_calls);
}

static void test_start_drives_both_off(void)
{
    s_relay[0].on = true;
    s_relay[1].on = true;
    TEST_ASSERT_TRUE(app_fsm_start(s_fsm));
    TEST_ASSERT_EQUAL(APP_FSM_STATE_READY, app_fsm_state(s_fsm));
    TEST_ASSERT_FALSE(s_relay[0].on);
    TEST_ASSERT_FALSE(s_relay[1].on);
    TEST_ASSERT_EQUAL_size_t(0, s_out_len); /* start is silent */
}

static void test_start_reports_safe_state_failure(void)
{
    s_relay[1].fail_with = RELAY_ERR_IO;
    TEST_ASSERT_FALSE(app_fsm_start(s_fsm));
    TEST_ASSERT_EQUAL_INT(1, s_relay[0].set_calls); /* still tried both */
}

static void test_hello(void)
{
    start_ready();
    line("HeyU");
    TEST_ASSERT_EQUAL_STRING("Hello IGNACC v1.2.3 READY\r\n", s_out);
}

static void test_status_reflects_relays(void)
{
    start_ready();
    line("STATUS");
    TEST_ASSERT_EQUAL_STRING("STATUS IN1=OFF IN2=OFF\r\n", s_out);
    clear_logs();
    s_relay[1].on = true;
    line("STATUS");
    TEST_ASSERT_EQUAL_STRING("STATUS IN1=OFF IN2=ON\r\n", s_out);
}

static void test_relay_on_sets_then_settles_then_replies(void)
{
    start_ready();
    line("IN1_ON");
    TEST_ASSERT_EQUAL_STRING("IN1_ON_OK\r\n", s_out);
    TEST_ASSERT_TRUE(s_relay[0].on);
    TEST_ASSERT_FALSE(s_relay[1].on);
    TEST_ASSERT_EQUAL_INT(3, s_event_count);
    TEST_ASSERT_EQUAL(EV_SET, s_events[0].kind);
    TEST_ASSERT_EQUAL(EV_DELAY, s_events[1].kind);
    TEST_ASSERT_EQUAL_INT(15, s_events[1].a);
    TEST_ASSERT_EQUAL(EV_SEND, s_events[2].kind);
}

static void test_all_relay_commands(void)
{
    start_ready();
    line("IN1_ON");
    line("IN2_ON");
    TEST_ASSERT_TRUE(s_relay[0].on);
    TEST_ASSERT_TRUE(s_relay[1].on);
    line("IN1_OFF");
    TEST_ASSERT_FALSE(s_relay[0].on);
    TEST_ASSERT_TRUE(s_relay[1].on);
    line("IN2_OFF");
    TEST_ASSERT_FALSE(s_relay[1].on);
    TEST_ASSERT_EQUAL_STRING("IN1_ON_OK\r\nIN2_ON_OK\r\nIN1_OFF_OK\r\nIN2_OFF_OK\r\n", s_out);
}

static void test_commands_are_idempotent(void)
{
    start_ready();
    line("IN2_ON");
    line("IN2_ON");
    line("IN2_OFF");
    line("IN2_OFF");
    TEST_ASSERT_EQUAL_INT(4, s_relay[1].set_calls); /* re-driven every time */
    TEST_ASSERT_EQUAL_STRING("IN2_ON_OK\r\nIN2_ON_OK\r\nIN2_OFF_OK\r\nIN2_OFF_OK\r\n", s_out);
}

static void test_set_failure_replies_fail_and_forces_safe_state(void)
{
    start_ready();
    line("IN2_ON");
    clear_logs();
    s_relay[0].fail_with = RELAY_ERR_VERIFY;
    line("IN1_ON");
    TEST_ASSERT_EQUAL_STRING("IN1_ON_FAIL\r\n", s_out);
    TEST_ASSERT_FALSE(s_relay[1].on); /* other relay forced OFF too */
    for (int i = 0; i < s_event_count; i++) {
        TEST_ASSERT_NOT_EQUAL(EV_DELAY, s_events[i].kind); /* FAIL is not delayed */
    }
}

static void test_io_failure_on_off_command(void)
{
    start_ready();
    s_relay[1].fail_with = RELAY_ERR_IO;
    line("IN2_OFF");
    TEST_ASSERT_EQUAL_STRING("IN2_OFF_FAIL\r\n", s_out);
}

static void test_readback_lost_after_settle_is_fail(void)
{
    start_ready();
    s_relay[0].drops_after_settle = true;
    line("IN1_ON");
    TEST_ASSERT_EQUAL_STRING("IN1_ON_FAIL\r\n", s_out);
    TEST_ASSERT_FALSE(s_relay[0].on);
}

static void test_unknown_lines_are_silent(void)
{
    start_ready();
    line("rst:0x1 (POWERON_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)");
    line("heyu");
    line("IN1_ON ");
    line("");
    app_fsm_handle_line(s_fsm, "IN1_ON\0", 7);
    TEST_ASSERT_EQUAL_size_t(0, s_out_len);
    TEST_ASSERT_EQUAL_INT(0, s_relay[0].set_calls);
    TEST_ASSERT_EQUAL_INT(0, s_relay[1].set_calls);
}

static void test_zero_settle_skips_delay(void)
{
    app_fsm_destroy(s_fsm);
    const app_fsm_config_t cfg = make_config(0);
    s_fsm = app_fsm_create(&cfg);
    start_ready();
    line("IN1_ON");
    TEST_ASSERT_EQUAL_STRING("IN1_ON_OK\r\n", s_out);
    for (int i = 0; i < s_event_count; i++) {
        TEST_ASSERT_NOT_EQUAL(EV_DELAY, s_events[i].kind);
    }
}

static void test_create_rejects_incomplete_config(void)
{
    app_fsm_config_t cfg = make_config(15);
    cfg.delay = NULL;
    TEST_ASSERT_NULL(app_fsm_create(&cfg));
    cfg = make_config(15);
    cfg.relays[1].ops = NULL;
    TEST_ASSERT_NULL(app_fsm_create(&cfg));
    cfg = make_config(15);
    cfg.link.ops = NULL;
    TEST_ASSERT_NULL(app_fsm_create(&cfg));
    cfg = make_config(15);
    cfg.fw_version = NULL;
    TEST_ASSERT_NULL(app_fsm_create(&cfg));
    TEST_ASSERT_NULL(app_fsm_create(NULL));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_state_ignores_lines);
    RUN_TEST(test_start_drives_both_off);
    RUN_TEST(test_start_reports_safe_state_failure);
    RUN_TEST(test_hello);
    RUN_TEST(test_status_reflects_relays);
    RUN_TEST(test_relay_on_sets_then_settles_then_replies);
    RUN_TEST(test_all_relay_commands);
    RUN_TEST(test_commands_are_idempotent);
    RUN_TEST(test_set_failure_replies_fail_and_forces_safe_state);
    RUN_TEST(test_io_failure_on_off_command);
    RUN_TEST(test_readback_lost_after_settle_is_fail);
    RUN_TEST(test_unknown_lines_are_silent);
    RUN_TEST(test_zero_settle_skips_delay);
    RUN_TEST(test_create_rejects_incomplete_config);
    return UNITY_END();
}
