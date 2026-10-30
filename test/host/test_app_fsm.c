#include <stdio.h>
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

static event_t s_events[256];
static int s_event_count;

static void log_event(event_kind_t kind, int a, int b)
{
    TEST_ASSERT_LESS_THAN_INT(256, s_event_count);
    s_events[s_event_count++] = (event_t){ kind, a, b };
}

static int count_events(event_kind_t kind)
{
    int n = 0;
    for (int i = 0; i < s_event_count; i++) {
        n += s_events[i].kind == kind;
    }
    return n;
}

/* ---- Mock relays -------------------------------------------------------- */

#define MAX_RELAYS PROTO_MAX_CHANNELS

typedef struct {
    int id;
    bool on;
    relay_err_t fail_with;   /* returned by set when != RELAY_OK */
    bool drops_after_settle; /* reads OFF after the settle delay */
    int set_calls;
} mock_relay_t;

static mock_relay_t s_relay[MAX_RELAYS];

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

static char s_out[2048];
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
    for (int i = 0; i < MAX_RELAYS; i++) {
        if (s_relay[i].drops_after_settle) {
            s_relay[i].on = false;
        }
    }
}

/* ---- Fixture ------------------------------------------------------------ */

static app_fsm_t *s_fsm;
static unsigned s_count;

static app_fsm_config_t make_config(unsigned count, uint32_t settle_ms)
{
    static relay_t handles[MAX_RELAYS];
    for (unsigned i = 0; i < MAX_RELAYS; i++) {
        handles[i] = (relay_t){ .ops = &s_relay_ops, .ctx = &s_relay[i] };
    }
    app_fsm_config_t cfg = {
        .relays = { .relays = handles, .count = count },
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
    for (int i = 0; i < MAX_RELAYS; i++) {
        s_relay[i].set_calls = 0;
    }
}

/* Recreate the FSM with `count` relays and the given settle time. */
static void use_fsm(unsigned count, uint32_t settle_ms)
{
    if (s_fsm != NULL) {
        app_fsm_destroy(s_fsm);
    }
    const app_fsm_config_t cfg = make_config(count, settle_ms);
    s_fsm = app_fsm_create(&cfg);
    TEST_ASSERT_NOT_NULL(s_fsm);
    s_count = count;
}

void setUp(void)
{
    memset(s_relay, 0, sizeof(s_relay));
    for (int i = 0; i < MAX_RELAYS; i++) {
        s_relay[i].id = i;
    }
    clear_logs();
    s_fsm = NULL;
    use_fsm(2, 15); /* the original two-channel board */
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

/* Expected STATUS line for the first s_count relays. */
static const char *expected_status(void)
{
    static char buf[PROTO_RESPONSE_MAX];
    int n = snprintf(buf, sizeof(buf), "STATUS");
    for (unsigned i = 0; i < s_count; i++) {
        n += snprintf(buf + n, sizeof(buf) - (size_t)n, " IN%u=%s", i + 1, s_relay[i].on ? "ON" : "OFF");
    }
    snprintf(buf + n, sizeof(buf) - (size_t)n, "\r\n");
    return buf;
}

static void assert_all(bool on)
{
    for (unsigned i = 0; i < s_count; i++) {
        TEST_ASSERT_EQUAL_MESSAGE(on, s_relay[i].on, "relay state");
    }
}

/* ---- Original two-channel behaviour (must not change) ------------------- */

static void test_init_state_ignores_lines(void)
{
    TEST_ASSERT_EQUAL(APP_FSM_STATE_INIT, app_fsm_state(s_fsm));
    line("HeyU");
    line("IN1_ON");
    line("ALL_ON");
    TEST_ASSERT_EQUAL_size_t(0, s_out_len);
    TEST_ASSERT_EQUAL_INT(0, s_relay[0].set_calls);
}

static void test_start_drives_all_off(void)
{
    s_relay[0].on = true;
    s_relay[1].on = true;
    TEST_ASSERT_TRUE(app_fsm_start(s_fsm));
    TEST_ASSERT_EQUAL(APP_FSM_STATE_READY, app_fsm_state(s_fsm));
    assert_all(false);
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
    TEST_ASSERT_EQUAL_INT(0, count_events(EV_DELAY)); /* FAIL is not delayed */
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

static void test_unknown_lines_get_nack(void)
{
    start_ready();
    line("rst:0x1 (POWERON_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)");
    line("heyu");
    line("IN1_ON ");
    line("");
    line("all_on");
    line("ALL_ON ");
    line("IN0_ON");
    line("IN01_ON");
    app_fsm_handle_line(s_fsm, "IN1_ON\0", 7);
    app_fsm_handle_line(s_fsm, "ALL_ON\0", 7);
    /* One UNK_NACK per line, nothing else sent, no relay touched. */
    char want[256] = "";
    for (int i = 0; i < 10; i++) {
        strcat(want, "UNK_NACK\r\n");
    }
    TEST_ASSERT_EQUAL_STRING(want, s_out);
    TEST_ASSERT_EQUAL_INT(10, count_events(EV_SEND));
    TEST_ASSERT_EQUAL_INT(0, count_events(EV_SET));
    TEST_ASSERT_EQUAL_INT(0, count_events(EV_DELAY));
}

static void test_nack_does_not_change_state(void)
{
    start_ready();
    line("IN1_ON");
    clear_logs();
    line("garbage");
    line("STATUS");
    TEST_ASSERT_EQUAL_STRING("UNK_NACK\r\nSTATUS IN1=ON IN2=OFF\r\n", s_out);
    TEST_ASSERT_TRUE(s_relay[0].on);
}

static void test_zero_settle_skips_delay(void)
{
    use_fsm(2, 0);
    start_ready();
    line("IN1_ON");
    line("ALL_ON");
    TEST_ASSERT_EQUAL_STRING("IN1_ON_OK\r\nALL_ON_OK\r\n", s_out);
    TEST_ASSERT_EQUAL_INT(0, count_events(EV_DELAY));
}

static void test_create_rejects_incomplete_config(void)
{
    app_fsm_config_t cfg = make_config(2, 15);
    cfg.delay = NULL;
    TEST_ASSERT_NULL(app_fsm_create(&cfg));
    cfg = make_config(2, 15);
    s_relay[0].id = 0;
    relay_t broken[2] = { { .ops = &s_relay_ops, .ctx = &s_relay[0] }, { .ops = NULL, .ctx = NULL } };
    cfg.relays.relays = broken;
    TEST_ASSERT_NULL(app_fsm_create(&cfg));
    cfg = make_config(2, 15);
    cfg.link.ops = NULL;
    TEST_ASSERT_NULL(app_fsm_create(&cfg));
    cfg = make_config(2, 15);
    cfg.fw_version = NULL;
    TEST_ASSERT_NULL(app_fsm_create(&cfg));
    cfg = make_config(0, 15);
    TEST_ASSERT_NULL(app_fsm_create(&cfg));
    cfg = make_config(PROTO_MAX_CHANNELS + 1, 15);
    TEST_ASSERT_NULL(app_fsm_create(&cfg));
    cfg = make_config(2, 15);
    cfg.relays.relays = NULL;
    TEST_ASSERT_NULL(app_fsm_create(&cfg));
    TEST_ASSERT_NULL(app_fsm_create(NULL));
}

/* ---- ALL_ON / ALL_OFF --------------------------------------------------- */

static void test_all_on_sets_every_relay_then_one_settle_then_replies(void)
{
    use_fsm(4, 15);
    start_ready();
    line("ALL_ON");
    TEST_ASSERT_EQUAL_STRING("ALL_ON_OK\r\n", s_out);
    assert_all(true);
    /* SET x4 (in channel order), one DELAY, one SEND. */
    TEST_ASSERT_EQUAL_INT(6, s_event_count);
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT_EQUAL(EV_SET, s_events[i].kind);
        TEST_ASSERT_EQUAL_INT(i, s_events[i].a);
        TEST_ASSERT_EQUAL_INT(1, s_events[i].b);
    }
    TEST_ASSERT_EQUAL(EV_DELAY, s_events[4].kind);
    TEST_ASSERT_EQUAL_INT(15, s_events[4].a);
    TEST_ASSERT_EQUAL(EV_SEND, s_events[5].kind);
}

static void test_all_off_from_all_on(void)
{
    use_fsm(4, 15);
    start_ready();
    line("ALL_ON");
    clear_logs();
    line("ALL_OFF");
    TEST_ASSERT_EQUAL_STRING("ALL_OFF_OK\r\n", s_out);
    assert_all(false);
    TEST_ASSERT_EQUAL_INT(1, count_events(EV_DELAY));
}

static void test_all_from_mixed_states(void)
{
    use_fsm(4, 15);
    start_ready();
    line("IN1_ON");
    line("IN3_ON");
    clear_logs();
    line("ALL_ON");
    assert_all(true);
    line("IN2_OFF");
    line("STATUS");
    line("ALL_OFF");
    assert_all(false);
    TEST_ASSERT_EQUAL_STRING("ALL_ON_OK\r\nIN2_OFF_OK\r\n"
                             "STATUS IN1=ON IN2=OFF IN3=ON IN4=ON\r\n"
                             "ALL_OFF_OK\r\n", s_out);
}

static void test_all_commands_are_idempotent(void)
{
    use_fsm(3, 15);
    start_ready();
    line("ALL_ON");
    line("ALL_ON");
    line("ALL_OFF");
    line("ALL_OFF");
    TEST_ASSERT_EQUAL_STRING("ALL_ON_OK\r\nALL_ON_OK\r\nALL_OFF_OK\r\nALL_OFF_OK\r\n", s_out);
    for (int i = 0; i < 3; i++) {
        TEST_ASSERT_EQUAL_INT(4, s_relay[i].set_calls); /* re-driven every time */
    }
}

static void test_all_on_failure_in_the_middle_forces_all_off(void)
{
    use_fsm(4, 15);
    start_ready();
    s_relay[2].fail_with = RELAY_ERR_VERIFY;
    line("ALL_ON");
    TEST_ASSERT_EQUAL_STRING("ALL_ON_FAIL\r\n", s_out);
    /* Relay 4 was still attempted, then everything was driven back OFF. */
    TEST_ASSERT_EQUAL_INT(2, s_relay[3].set_calls);
    TEST_ASSERT_FALSE(s_relay[0].on);
    TEST_ASSERT_FALSE(s_relay[1].on);
    TEST_ASSERT_FALSE(s_relay[3].on);
    TEST_ASSERT_EQUAL_INT(0, count_events(EV_DELAY)); /* FAIL is not delayed */
}

static void test_all_on_readback_lost_after_settle_is_fail(void)
{
    use_fsm(4, 15);
    start_ready();
    s_relay[3].drops_after_settle = true;
    line("ALL_ON");
    TEST_ASSERT_EQUAL_STRING("ALL_ON_FAIL\r\n", s_out);
    assert_all(false);
}

static void test_all_off_failure_reports_fail(void)
{
    use_fsm(4, 15);
    start_ready();
    line("ALL_ON");
    clear_logs();
    s_relay[1].fail_with = RELAY_ERR_IO;
    line("ALL_OFF");
    TEST_ASSERT_EQUAL_STRING("ALL_OFF_FAIL\r\n", s_out);
    TEST_ASSERT_FALSE(s_relay[0].on);
    TEST_ASSERT_FALSE(s_relay[2].on);
    TEST_ASSERT_FALSE(s_relay[3].on);
}

static void test_all_does_not_touch_relays_beyond_count(void)
{
    use_fsm(2, 15);
    start_ready();
    line("ALL_ON");
    TEST_ASSERT_EQUAL_INT(0, s_relay[2].set_calls);
    TEST_ASSERT_FALSE(s_relay[2].on);
}

/* ---- Channel count handling --------------------------------------------- */

static void test_channels_beyond_count_get_nack(void)
{
    use_fsm(2, 15);
    start_ready();
    line("IN3_ON");   /* valid grammar, channel not on this board */
    line("IN16_OFF");
    line("IN17_ON");  /* outside the grammar */
    TEST_ASSERT_EQUAL_STRING("UNK_NACK\r\nUNK_NACK\r\nUNK_NACK\r\n", s_out);
    TEST_ASSERT_EQUAL_INT(0, s_relay[2].set_calls);
    TEST_ASSERT_EQUAL_INT(0, count_events(EV_SET));
}

static void test_single_channel_board(void)
{
    use_fsm(1, 15);
    start_ready();
    TEST_ASSERT_EQUAL_UINT(1, app_fsm_channel_count(s_fsm));
    line("STATUS");
    line("IN1_ON");
    line("IN2_ON"); /* not on this board */
    line("STATUS");
    line("ALL_OFF");
    TEST_ASSERT_EQUAL_STRING("STATUS IN1=OFF\r\nIN1_ON_OK\r\nUNK_NACK\r\nSTATUS IN1=ON\r\nALL_OFF_OK\r\n", s_out);
}

static void test_max_channel_board(void)
{
    use_fsm(PROTO_MAX_CHANNELS, 15);
    start_ready();
    TEST_ASSERT_EQUAL_UINT(PROTO_MAX_CHANNELS, app_fsm_channel_count(s_fsm));
    line("IN16_ON");
    TEST_ASSERT_TRUE(s_relay[15].on);
    clear_logs();
    line("STATUS");
    TEST_ASSERT_EQUAL_STRING(expected_status(), s_out);
    clear_logs();
    line("ALL_ON");
    assert_all(true);
    line("ALL_OFF");
    assert_all(false);
    TEST_ASSERT_EQUAL_STRING("ALL_ON_OK\r\nALL_OFF_OK\r\n", s_out);
}

static void test_status_matches_every_count(void)
{
    for (unsigned count = 1; count <= PROTO_MAX_CHANNELS; count++) {
        use_fsm(count, 0);
        start_ready();
        for (unsigned i = 0; i < count; i += 2) {
            s_relay[i].on = true; /* odd channels ON */
        }
        line("STATUS");
        TEST_ASSERT_EQUAL_STRING(expected_status(), s_out);
    }
}

static void test_fsm_keeps_its_own_copy_of_relay_handles(void)
{
    relay_t handles[3];
    for (int i = 0; i < 3; i++) {
        handles[i] = (relay_t){ .ops = &s_relay_ops, .ctx = &s_relay[i] };
    }
    app_fsm_config_t cfg = make_config(3, 15);
    cfg.relays.relays = handles;
    app_fsm_destroy(s_fsm);
    s_fsm = app_fsm_create(&cfg);
    s_count = 3;
    memset(handles, 0, sizeof(handles)); /* caller's array goes away */
    start_ready();
    line("ALL_ON");
    TEST_ASSERT_EQUAL_STRING("ALL_ON_OK\r\n", s_out);
    assert_all(true);
}

int main(void)
{
    UNITY_BEGIN();
    /* original behaviour */
    RUN_TEST(test_init_state_ignores_lines);
    RUN_TEST(test_start_drives_all_off);
    RUN_TEST(test_start_reports_safe_state_failure);
    RUN_TEST(test_hello);
    RUN_TEST(test_status_reflects_relays);
    RUN_TEST(test_relay_on_sets_then_settles_then_replies);
    RUN_TEST(test_all_relay_commands);
    RUN_TEST(test_commands_are_idempotent);
    RUN_TEST(test_set_failure_replies_fail_and_forces_safe_state);
    RUN_TEST(test_io_failure_on_off_command);
    RUN_TEST(test_readback_lost_after_settle_is_fail);
    RUN_TEST(test_unknown_lines_get_nack);
    RUN_TEST(test_nack_does_not_change_state);
    RUN_TEST(test_zero_settle_skips_delay);
    RUN_TEST(test_create_rejects_incomplete_config);
    /* ALL_ON / ALL_OFF */
    RUN_TEST(test_all_on_sets_every_relay_then_one_settle_then_replies);
    RUN_TEST(test_all_off_from_all_on);
    RUN_TEST(test_all_from_mixed_states);
    RUN_TEST(test_all_commands_are_idempotent);
    RUN_TEST(test_all_on_failure_in_the_middle_forces_all_off);
    RUN_TEST(test_all_on_readback_lost_after_settle_is_fail);
    RUN_TEST(test_all_off_failure_reports_fail);
    RUN_TEST(test_all_does_not_touch_relays_beyond_count);
    /* channel count */
    RUN_TEST(test_channels_beyond_count_get_nack);
    RUN_TEST(test_single_channel_board);
    RUN_TEST(test_max_channel_board);
    RUN_TEST(test_status_matches_every_count);
    RUN_TEST(test_fsm_keeps_its_own_copy_of_relay_handles);
    return UNITY_END();
}
