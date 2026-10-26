#include <string.h>

#include "line_assembler.h"
#include "unity.h"

#define MAX_LEN 32

static line_asm_t s_asm;
static link_line_t s_lines[8];
static int s_count;

void setUp(void)
{
    line_asm_init(&s_asm, MAX_LEN);
    s_count = 0;
}

void tearDown(void) {}

static void feed(const char *bytes, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        link_line_t line;
        if (line_asm_push(&s_asm, bytes[i], &line)) {
            TEST_ASSERT_LESS_THAN_INT(8, s_count);
            s_lines[s_count++] = line;
        }
    }
}

static void feed_str(const char *s)
{
    feed(s, strlen(s));
}

static void assert_line(int index, const char *expected)
{
    TEST_ASSERT_LESS_THAN_INT(s_count, index);
    TEST_ASSERT_EQUAL_UINT16(strlen(expected), s_lines[index].len);
    TEST_ASSERT_EQUAL_MEMORY(expected, s_lines[index].data, strlen(expected));
}

static void test_emits_line_on_newline(void)
{
    feed_str("IN1_ON");
    TEST_ASSERT_EQUAL_INT(0, s_count);
    feed_str("\n");
    TEST_ASSERT_EQUAL_INT(1, s_count);
    assert_line(0, "IN1_ON");
}

static void test_strips_trailing_cr(void)
{
    feed_str("HeyU\r\n");
    TEST_ASSERT_EQUAL_INT(1, s_count);
    assert_line(0, "HeyU");
}

static void test_strips_only_one_cr(void)
{
    feed_str("HeyU\r\r\n");
    TEST_ASSERT_EQUAL_INT(1, s_count);
    assert_line(0, "HeyU\r");
}

static void test_ignores_empty_lines(void)
{
    feed_str("\n\r\n\n");
    TEST_ASSERT_EQUAL_INT(0, s_count);
}

static void test_multiple_lines_in_one_chunk(void)
{
    feed_str("IN1_ON\r\nSTATUS\nIN2_OFF\r\n");
    TEST_ASSERT_EQUAL_INT(3, s_count);
    assert_line(0, "IN1_ON");
    assert_line(1, "STATUS");
    assert_line(2, "IN2_OFF");
}

static void test_accepts_exactly_max_len(void)
{
    char line[MAX_LEN + 3];
    memset(line, 'A', MAX_LEN);
    line[MAX_LEN] = '\n';
    feed(line, MAX_LEN + 1);
    TEST_ASSERT_EQUAL_INT(1, s_count);
    TEST_ASSERT_EQUAL_UINT16(MAX_LEN, s_lines[0].len);

    line[MAX_LEN] = '\r';
    line[MAX_LEN + 1] = '\n';
    feed(line, MAX_LEN + 2);
    TEST_ASSERT_EQUAL_INT(2, s_count);
    TEST_ASSERT_EQUAL_UINT16(MAX_LEN, s_lines[1].len);
}

static void test_discards_max_len_plus_one(void)
{
    char line[MAX_LEN + 3];
    memset(line, 'A', MAX_LEN + 1);
    line[MAX_LEN + 1] = '\n';
    feed(line, MAX_LEN + 2);
    TEST_ASSERT_EQUAL_INT(0, s_count);

    line[MAX_LEN + 1] = '\r';
    line[MAX_LEN + 2] = '\n';
    feed(line, MAX_LEN + 3);
    TEST_ASSERT_EQUAL_INT(0, s_count);
}

static void test_long_line_discarded_entirely_then_recovers(void)
{
    char junk[200];
    memset(junk, 'X', sizeof(junk));
    feed(junk, sizeof(junk));
    /* A command at the tail of a long line must not leak through. */
    feed_str("IN1_ON\r\n");
    TEST_ASSERT_EQUAL_INT(0, s_count);

    feed_str("IN1_ON\r\n");
    TEST_ASSERT_EQUAL_INT(1, s_count);
    assert_line(0, "IN1_ON");
}

static void test_discard_drops_until_next_newline(void)
{
    feed_str("IN1_");
    line_asm_discard(&s_asm);
    feed_str("ON\r\n");
    TEST_ASSERT_EQUAL_INT(0, s_count);
    feed_str("IN1_ON\n");
    TEST_ASSERT_EQUAL_INT(1, s_count);
    assert_line(0, "IN1_ON");
}

static void test_rom_noise_then_command(void)
{
    feed_str("ets Jul 29 2019 12:21:46\r\n\r\nrst:0x1 (POWERON_RESET),boot:0x13\r\n");
    feed_str("HeyU\r\n");
    /* "ets ..." is emitted (and later ignored by the parser); the 33-char
     * "rst:..." line exceeds MAX_LEN and is dropped here already. */
    TEST_ASSERT_EQUAL_INT(2, s_count);
    assert_line(1, "HeyU");
}

static void test_max_len_is_clamped(void)
{
    line_asm_init(&s_asm, 1000);
    TEST_ASSERT_EQUAL_size_t(LINK_LINE_MAX_LEN, s_asm.max_len);
    line_asm_init(&s_asm, 0);
    TEST_ASSERT_EQUAL_size_t(1, s_asm.max_len);
}

static void test_upper_bound_line_fits(void)
{
    char line[LINK_LINE_MAX_LEN + 2];
    line_asm_init(&s_asm, LINK_LINE_MAX_LEN);
    memset(line, 'B', LINK_LINE_MAX_LEN);
    line[LINK_LINE_MAX_LEN] = '\r';
    line[LINK_LINE_MAX_LEN + 1] = '\n';
    feed(line, sizeof(line));
    TEST_ASSERT_EQUAL_INT(1, s_count);
    TEST_ASSERT_EQUAL_UINT16(LINK_LINE_MAX_LEN, s_lines[0].len);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_emits_line_on_newline);
    RUN_TEST(test_strips_trailing_cr);
    RUN_TEST(test_strips_only_one_cr);
    RUN_TEST(test_ignores_empty_lines);
    RUN_TEST(test_multiple_lines_in_one_chunk);
    RUN_TEST(test_accepts_exactly_max_len);
    RUN_TEST(test_discards_max_len_plus_one);
    RUN_TEST(test_long_line_discarded_entirely_then_recovers);
    RUN_TEST(test_discard_drops_until_next_newline);
    RUN_TEST(test_rom_noise_then_command);
    RUN_TEST(test_max_len_is_clamped);
    RUN_TEST(test_upper_bound_line_fits);
    return UNITY_END();
}
