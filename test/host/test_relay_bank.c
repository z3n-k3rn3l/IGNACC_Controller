#include <string.h>

#include "esp32_relay_pins.h"
#include "relay_bank.h"
#include "unity.h"

/* ---- Mock relays ---------------------------------------------------------- */

#define MAX_MOCKS 16

typedef struct {
    bool on;
    relay_err_t fail_with;
    int set_calls;
    int order; /* sequence number of the last set call */
} mock_relay_t;

static mock_relay_t s_mock[MAX_MOCKS];
static relay_t s_relays[MAX_MOCKS];
static int s_seq;

static relay_err_t mock_set(void *ctx, bool on)
{
    mock_relay_t *m = ctx;
    m->set_calls++;
    m->order = ++s_seq;
    if (m->fail_with != RELAY_OK) {
        return m->fail_with;
    }
    m->on = on;
    return RELAY_OK;
}

static bool mock_is_on(void *ctx)
{
    return ((mock_relay_t *)ctx)->on;
}

static const relay_ops_t s_ops = { .set = mock_set, .is_on = mock_is_on };

void setUp(void)
{
    memset(s_mock, 0, sizeof(s_mock));
    s_seq = 0;
    for (int i = 0; i < MAX_MOCKS; i++) {
        s_relays[i] = (relay_t){ .ops = &s_ops, .ctx = &s_mock[i] };
    }
}

void tearDown(void) {}

static relay_bank_t bank(unsigned count)
{
    return (relay_bank_t){ .relays = s_relays, .count = count };
}

/* ---- Tests ---------------------------------------------------------------- */

static void test_set_and_read_by_index(void)
{
    relay_bank_t b = bank(3);
    TEST_ASSERT_EQUAL_UINT(3, relay_bank_count(&b));
    TEST_ASSERT_EQUAL(RELAY_OK, relay_bank_set(&b, 1, true));
    TEST_ASSERT_FALSE(relay_bank_is_on(&b, 0));
    TEST_ASSERT_TRUE(relay_bank_is_on(&b, 1));
    TEST_ASSERT_FALSE(relay_bank_is_on(&b, 2));
}

static void test_index_out_of_range(void)
{
    relay_bank_t b = bank(2);
    TEST_ASSERT_EQUAL(RELAY_ERR_ARG, relay_bank_set(&b, 2, true));
    TEST_ASSERT_FALSE(relay_bank_is_on(&b, 2));
    TEST_ASSERT_EQUAL_INT(0, s_mock[2].set_calls); /* never touched */
    TEST_ASSERT_EQUAL(RELAY_ERR_ARG, relay_bank_set(NULL, 0, true));
}

static void test_set_all_drives_every_relay_in_order(void)
{
    for (unsigned count = 1; count <= MAX_MOCKS; count++) {
        setUp();
        relay_bank_t b = bank(count);
        TEST_ASSERT_EQUAL(RELAY_OK, relay_bank_set_all(&b, true));
        TEST_ASSERT_TRUE(relay_bank_all_are(&b, true));
        for (unsigned i = 0; i < count; i++) {
            TEST_ASSERT_EQUAL_INT(1, s_mock[i].set_calls);
            TEST_ASSERT_EQUAL_INT((int)i + 1, s_mock[i].order);
        }
        if (count < MAX_MOCKS) {
            TEST_ASSERT_EQUAL_INT(0, s_mock[count].set_calls); /* stays within the bank */
        }
        TEST_ASSERT_EQUAL(RELAY_OK, relay_bank_set_all(&b, false));
        TEST_ASSERT_TRUE(relay_bank_all_are(&b, false));
    }
}

static void test_set_all_continues_after_a_failure(void)
{
    relay_bank_t b = bank(4);
    s_mock[1].fail_with = RELAY_ERR_VERIFY;
    s_mock[2].fail_with = RELAY_ERR_IO;
    TEST_ASSERT_EQUAL(RELAY_ERR_VERIFY, relay_bank_set_all(&b, true)); /* first error wins */
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT_EQUAL_INT(1, s_mock[i].set_calls); /* every relay was attempted */
    }
    TEST_ASSERT_TRUE(s_mock[0].on);
    TEST_ASSERT_TRUE(s_mock[3].on);
    TEST_ASSERT_FALSE(relay_bank_all_are(&b, true));
}

static void test_all_are_detects_one_mismatch(void)
{
    relay_bank_t b = bank(5);
    relay_bank_set_all(&b, true);
    s_mock[4].on = false;
    TEST_ASSERT_FALSE(relay_bank_all_are(&b, true));
    TEST_ASSERT_FALSE(relay_bank_all_are(&b, false));
    TEST_ASSERT_FALSE(relay_bank_all_are(NULL, false));
}

static void test_set_all_null_bank(void)
{
    TEST_ASSERT_EQUAL(RELAY_ERR_ARG, relay_bank_set_all(NULL, false));
}

static void test_esp32_safe_pin_list(void)
{
    const int expected[] = { 4, 13, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33 };
    int safe = 0;
    for (int p = 0; p < 40; p++) {
        bool in_list = false;
        for (size_t i = 0; i < sizeof(expected) / sizeof(expected[0]); i++) {
            in_list |= expected[i] == p;
        }
        TEST_ASSERT_EQUAL_MESSAGE(in_list, ESP32_RELAY_PIN_IS_SAFE(p), "GPIO safety mismatch");
        safe += ESP32_RELAY_PIN_IS_SAFE(p) ? 1 : 0;
    }
    TEST_ASSERT_EQUAL_INT(ESP32_RELAY_PIN_SAFE_COUNT, safe);
    /* The ones that must never be used. */
    const int forbidden[] = { 0, 1, 2, 3, 5, 6, 7, 8, 9, 10, 11, 12, 14, 15, 34, 35, 36, 37, 38, 39 };
    for (size_t i = 0; i < sizeof(forbidden) / sizeof(forbidden[0]); i++) {
        TEST_ASSERT_FALSE(ESP32_RELAY_PIN_IS_SAFE(forbidden[i]));
    }
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_set_and_read_by_index);
    RUN_TEST(test_index_out_of_range);
    RUN_TEST(test_set_all_drives_every_relay_in_order);
    RUN_TEST(test_set_all_continues_after_a_failure);
    RUN_TEST(test_all_are_detects_one_mismatch);
    RUN_TEST(test_set_all_null_bank);
    RUN_TEST(test_esp32_safe_pin_list);
    return UNITY_END();
}
