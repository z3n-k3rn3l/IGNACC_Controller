#include <string.h>

#include "relay_gpio.h"
#include "unity.h"

/* ---- Mock pin HAL ------------------------------------------------------- */

#define PINS 40

typedef struct {
    int level[PINS];      /* latched output level */
    bool output[PINS];    /* output driver enabled */
    int stuck[PINS];      /* -1: follows level, else forced pad level */
    int initial_at_enable[PINS];
    int fail_configure;
    int fail_set;
    int set_calls;
} mock_hal_t;

static mock_hal_t s_mock;

static int mock_configure_output(void *ctx, int pin, int initial_level)
{
    mock_hal_t *m = ctx;
    if (m->fail_configure) {
        return -1;
    }
    m->level[pin] = initial_level; /* latched before enabling */
    m->initial_at_enable[pin] = m->level[pin];
    m->output[pin] = true;
    return 0;
}

static int mock_set_level(void *ctx, int pin, int level)
{
    mock_hal_t *m = ctx;
    m->set_calls++;
    if (m->fail_set) {
        return -1;
    }
    m->level[pin] = level;
    return 0;
}

static int mock_get_level(void *ctx, int pin)
{
    mock_hal_t *m = ctx;
    return m->stuck[pin] >= 0 ? m->stuck[pin] : m->level[pin];
}

static const pin_hal_ops_t s_mock_ops = {
    .configure_output = mock_configure_output,
    .set_level = mock_set_level,
    .get_level = mock_get_level,
};

static pin_hal_t s_hal = { .ops = &s_mock_ops, .ctx = &s_mock };
static relay_t s_relay;

void setUp(void)
{
    memset(&s_mock, 0, sizeof(s_mock));
    for (int i = 0; i < PINS; i++) {
        s_mock.stuck[i] = -1;
        s_mock.initial_at_enable[i] = -1;
    }
    memset(&s_relay, 0, sizeof(s_relay));
}

void tearDown(void)
{
    relay_gpio_destroy(&s_relay);
}

static const relay_gpio_config_t ACTIVE_LOW_26 = { .pin = 26, .active_level = 0 };

/* ---- Tests -------------------------------------------------------------- */

static void test_create_latches_off_before_output_enable(void)
{
    TEST_ASSERT_EQUAL(RELAY_OK, relay_gpio_create(&ACTIVE_LOW_26, &s_hal, &s_relay));
    TEST_ASSERT_TRUE(s_mock.output[26]);
    TEST_ASSERT_EQUAL_INT(1, s_mock.initial_at_enable[26]); /* HIGH = OFF for active-low */
    TEST_ASSERT_EQUAL_INT(1, s_mock.level[26]);
    TEST_ASSERT_FALSE(relay_is_on(&s_relay));
}

static void test_active_low_on_drives_low(void)
{
    TEST_ASSERT_EQUAL(RELAY_OK, relay_gpio_create(&ACTIVE_LOW_26, &s_hal, &s_relay));
    TEST_ASSERT_EQUAL(RELAY_OK, relay_set(&s_relay, true));
    TEST_ASSERT_EQUAL_INT(0, s_mock.level[26]);
    TEST_ASSERT_TRUE(relay_is_on(&s_relay));
    TEST_ASSERT_EQUAL(RELAY_OK, relay_set(&s_relay, false));
    TEST_ASSERT_EQUAL_INT(1, s_mock.level[26]);
    TEST_ASSERT_FALSE(relay_is_on(&s_relay));
}

static void test_active_high(void)
{
    const relay_gpio_config_t cfg = { .pin = 27, .active_level = 1 };
    TEST_ASSERT_EQUAL(RELAY_OK, relay_gpio_create(&cfg, &s_hal, &s_relay));
    TEST_ASSERT_EQUAL_INT(0, s_mock.initial_at_enable[27]);
    TEST_ASSERT_EQUAL(RELAY_OK, relay_set(&s_relay, true));
    TEST_ASSERT_EQUAL_INT(1, s_mock.level[27]);
}

static void test_set_is_idempotent_and_redrives(void)
{
    TEST_ASSERT_EQUAL(RELAY_OK, relay_gpio_create(&ACTIVE_LOW_26, &s_hal, &s_relay));
    TEST_ASSERT_EQUAL(RELAY_OK, relay_set(&s_relay, true));
    TEST_ASSERT_EQUAL(RELAY_OK, relay_set(&s_relay, true));
    TEST_ASSERT_EQUAL_INT(2, s_mock.set_calls);
}

static void test_set_reports_io_error(void)
{
    TEST_ASSERT_EQUAL(RELAY_OK, relay_gpio_create(&ACTIVE_LOW_26, &s_hal, &s_relay));
    s_mock.fail_set = 1;
    TEST_ASSERT_EQUAL(RELAY_ERR_IO, relay_set(&s_relay, true));
}

static void test_set_reports_readback_mismatch(void)
{
    TEST_ASSERT_EQUAL(RELAY_OK, relay_gpio_create(&ACTIVE_LOW_26, &s_hal, &s_relay));
    s_mock.stuck[26] = 1; /* pad held HIGH externally */
    TEST_ASSERT_EQUAL(RELAY_ERR_VERIFY, relay_set(&s_relay, true));
    TEST_ASSERT_FALSE(relay_is_on(&s_relay));
}

static void test_create_fails_on_configure_error(void)
{
    s_mock.fail_configure = 1;
    TEST_ASSERT_EQUAL(RELAY_ERR_IO, relay_gpio_create(&ACTIVE_LOW_26, &s_hal, &s_relay));
    TEST_ASSERT_NULL(s_relay.ops);
}

static void test_create_fails_on_readback_mismatch(void)
{
    s_mock.stuck[26] = 0;
    TEST_ASSERT_EQUAL(RELAY_ERR_VERIFY, relay_gpio_create(&ACTIVE_LOW_26, &s_hal, &s_relay));
    TEST_ASSERT_NULL(s_relay.ops);
}

static void test_create_rejects_bad_arguments(void)
{
    const relay_gpio_config_t bad_level = { .pin = 26, .active_level = 2 };
    const relay_gpio_config_t bad_pin = { .pin = -1, .active_level = 0 };
    TEST_ASSERT_EQUAL(RELAY_ERR_ARG, relay_gpio_create(NULL, &s_hal, &s_relay));
    TEST_ASSERT_EQUAL(RELAY_ERR_ARG, relay_gpio_create(&ACTIVE_LOW_26, NULL, &s_relay));
    TEST_ASSERT_EQUAL(RELAY_ERR_ARG, relay_gpio_create(&ACTIVE_LOW_26, &s_hal, NULL));
    TEST_ASSERT_EQUAL(RELAY_ERR_ARG, relay_gpio_create(&bad_level, &s_hal, &s_relay));
    TEST_ASSERT_EQUAL(RELAY_ERR_ARG, relay_gpio_create(&bad_pin, &s_hal, &s_relay));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_create_latches_off_before_output_enable);
    RUN_TEST(test_active_low_on_drives_low);
    RUN_TEST(test_active_high);
    RUN_TEST(test_set_is_idempotent_and_redrives);
    RUN_TEST(test_set_reports_io_error);
    RUN_TEST(test_set_reports_readback_mismatch);
    RUN_TEST(test_create_fails_on_configure_error);
    RUN_TEST(test_create_fails_on_readback_mismatch);
    RUN_TEST(test_create_rejects_bad_arguments);
    return UNITY_END();
}
