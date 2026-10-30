#include <stdio.h>
#include <string.h>

#include "protocol.h"
#include "unity.h"

void setUp(void) {}
void tearDown(void) {}

static proto_cmd_t parse(const char *s)
{
    return proto_parse(s, strlen(s));
}

static void assert_relay(const char *line, unsigned channel, bool on)
{
    proto_cmd_t c = parse(line);
    TEST_ASSERT_EQUAL_MESSAGE(PROTO_CMD_RELAY, c.kind, line);
    TEST_ASSERT_EQUAL_UINT_MESSAGE(channel, c.channel, line);
    TEST_ASSERT_EQUAL_MESSAGE(on, c.on, line);
}

static void assert_none(const char *line)
{
    TEST_ASSERT_EQUAL_MESSAGE(PROTO_CMD_NONE, parse(line).kind, line);
}

static void test_parses_fixed_commands(void)
{
    TEST_ASSERT_EQUAL(PROTO_CMD_HELLO, parse("HeyU").kind);
    TEST_ASSERT_EQUAL(PROTO_CMD_STATUS, parse("STATUS").kind);
    proto_cmd_t all_on = parse("ALL_ON");
    TEST_ASSERT_EQUAL(PROTO_CMD_ALL, all_on.kind);
    TEST_ASSERT_TRUE(all_on.on);
    proto_cmd_t all_off = parse("ALL_OFF");
    TEST_ASSERT_EQUAL(PROTO_CMD_ALL, all_off.kind);
    TEST_ASSERT_FALSE(all_off.on);
}

static void test_parses_original_relay_commands(void)
{
    assert_relay("IN1_ON", 1, true);
    assert_relay("IN1_OFF", 1, false);
    assert_relay("IN2_ON", 2, true);
    assert_relay("IN2_OFF", 2, false);
}

static void test_parses_every_channel_up_to_max(void)
{
    char line[16];
    for (unsigned ch = 1; ch <= PROTO_MAX_CHANNELS; ch++) {
        snprintf(line, sizeof(line), "IN%u_ON", ch);
        assert_relay(line, ch, true);
        snprintf(line, sizeof(line), "IN%u_OFF", ch);
        assert_relay(line, ch, false);
    }
}

static void test_rejects_bad_channel_numbers(void)
{
    assert_none("IN0_ON");
    assert_none("IN01_ON");
    assert_none("IN001_OFF");
    assert_none("IN17_ON");
    assert_none("IN99_ON");
    assert_none("IN100_ON");
    assert_none("IN4294967297_ON"); /* would overflow a 32-bit accumulator */
    assert_none("IN_ON");
    assert_none("IN-1_ON");
    assert_none("IN+1_ON");
    assert_none("IN 1_ON");
    assert_none("IN1");
    assert_none("IN1_");
    assert_none("IN1_O");
    assert_none("IN1_ONN");
    assert_none("IN1_OFFF");
    assert_none("IN1_on");
}

static void test_is_case_sensitive(void)
{
    assert_none("in1_on");
    assert_none("heyu");
    assert_none("HEYU");
    assert_none("status");
    assert_none("all_on");
    assert_none("All_On");
    assert_none("ALL_on");
}

static void test_rejects_near_misses(void)
{
    assert_none("");
    assert_none("IN1_ONX");
    assert_none(" IN1_ON");
    assert_none("IN1_ON ");
    assert_none("IN1_ON\r");
    assert_none("ALL");
    assert_none("ALL_");
    assert_none("ALL_ONX");
    assert_none("ALL_ON ");
    assert_none(" ALL_OFF");
    assert_none("ALL_OF");
    assert_none("ALLON");
    TEST_ASSERT_EQUAL(PROTO_CMD_NONE, proto_parse(NULL, 0).kind);
}

static void test_embedded_nul_never_matches(void)
{
    TEST_ASSERT_EQUAL(PROTO_CMD_NONE, proto_parse("IN1_ON\0x", 8).kind);
    TEST_ASSERT_EQUAL(PROTO_CMD_NONE, proto_parse("IN1_ON\0", 7).kind);
    TEST_ASSERT_EQUAL(PROTO_CMD_NONE, proto_parse("ALL_ON\0", 7).kind);
    TEST_ASSERT_EQUAL(PROTO_CMD_NONE, proto_parse("IN\0_ON", 6).kind);
}

static void test_ignores_rom_bootloader_text(void)
{
    assert_none("rst:0x1 (POWERON_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)");
    assert_none("ets Jul 29 2019 12:21:46");
    assert_none("entry 0x400805e4");
}

static void assert_result(proto_cmd_t cmd, bool ok, const char *expected)
{
    char buf[PROTO_RESPONSE_MAX];
    int n = proto_format_result(&cmd, ok, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_INT((int)strlen(expected), n);
    TEST_ASSERT_EQUAL_STRING(expected, buf);
}

static void test_formats_relay_results_unchanged(void)
{
    assert_result(parse("IN1_ON"), true, "IN1_ON_OK\r\n");
    assert_result(parse("IN1_ON"), false, "IN1_ON_FAIL\r\n");
    assert_result(parse("IN1_OFF"), true, "IN1_OFF_OK\r\n");
    assert_result(parse("IN1_OFF"), false, "IN1_OFF_FAIL\r\n");
    assert_result(parse("IN2_ON"), true, "IN2_ON_OK\r\n");
    assert_result(parse("IN2_ON"), false, "IN2_ON_FAIL\r\n");
    assert_result(parse("IN2_OFF"), true, "IN2_OFF_OK\r\n");
    assert_result(parse("IN2_OFF"), false, "IN2_OFF_FAIL\r\n");
    assert_result(parse("IN16_OFF"), false, "IN16_OFF_FAIL\r\n");
}

static void test_formats_all_results(void)
{
    assert_result(parse("ALL_ON"), true, "ALL_ON_OK\r\n");
    assert_result(parse("ALL_ON"), false, "ALL_ON_FAIL\r\n");
    assert_result(parse("ALL_OFF"), true, "ALL_OFF_OK\r\n");
    assert_result(parse("ALL_OFF"), false, "ALL_OFF_FAIL\r\n");
}

static void test_result_rejects_non_relay_commands(void)
{
    char buf[PROTO_RESPONSE_MAX];
    proto_cmd_t hello = parse("HeyU");
    proto_cmd_t none = parse("nope");
    proto_cmd_t bad_channel = { PROTO_CMD_RELAY, 0, true };
    proto_cmd_t big_channel = { PROTO_CMD_RELAY, PROTO_MAX_CHANNELS + 1, true };
    TEST_ASSERT_EQUAL_INT(-1, proto_format_result(&hello, true, buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_INT(-1, proto_format_result(&none, true, buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_INT(-1, proto_format_result(&bad_channel, true, buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_INT(-1, proto_format_result(&big_channel, true, buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_INT(-1, proto_format_result(NULL, true, buf, sizeof(buf)));
}

static void test_formats_hello(void)
{
    char buf[PROTO_RESPONSE_MAX];
    int n = proto_format_hello("1.0.0", buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("Hello IGNACC v1.0.0 READY\r\n", buf);
    TEST_ASSERT_EQUAL_INT((int)strlen(buf), n);
}

static void test_hello_fits_longest_version(void)
{
    char version[32];
    char buf[PROTO_RESPONSE_MAX];
    memset(version, 'v', sizeof(version) - 1);
    version[sizeof(version) - 1] = '\0';
    TEST_ASSERT_GREATER_THAN_INT(0, proto_format_hello(version, buf, sizeof(buf)));
}

static void test_formats_nack(void)
{
    char buf[PROTO_RESPONSE_MAX];
    int n = proto_format_nack(buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("UNK_NACK\r\n", buf);
    TEST_ASSERT_EQUAL_INT(10, n);
    char tight[10]; /* one byte short for the NUL */
    TEST_ASSERT_EQUAL_INT(-1, proto_format_nack(tight, sizeof(tight)));
    TEST_ASSERT_EQUAL_INT(-1, proto_format_nack(NULL, 0));
}

static void test_formats_two_channel_status_unchanged(void)
{
    char buf[PROTO_RESPONSE_MAX];
    bool s[2];
    s[0] = false; s[1] = false;
    proto_format_status(s, 2, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("STATUS IN1=OFF IN2=OFF\r\n", buf);
    s[0] = true; s[1] = false;
    proto_format_status(s, 2, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("STATUS IN1=ON IN2=OFF\r\n", buf);
    s[0] = false; s[1] = true;
    proto_format_status(s, 2, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("STATUS IN1=OFF IN2=ON\r\n", buf);
    s[0] = true; s[1] = true;
    proto_format_status(s, 2, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("STATUS IN1=ON IN2=ON\r\n", buf);
}

static void test_formats_status_any_count(void)
{
    char buf[PROTO_RESPONSE_MAX];
    bool s[3] = { true, false, true };
    int n = proto_format_status(s, 1, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("STATUS IN1=ON\r\n", buf);
    TEST_ASSERT_EQUAL_INT((int)strlen(buf), n);
    proto_format_status(s, 3, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("STATUS IN1=ON IN2=OFF IN3=ON\r\n", buf);
}

static void test_max_status_fits_response_buffer(void)
{
    char buf[PROTO_RESPONSE_MAX];
    bool s[PROTO_MAX_CHANNELS];
    for (unsigned i = 0; i < PROTO_MAX_CHANNELS; i++) {
        s[i] = false; /* "OFF" is the longest state */
    }
    int n = proto_format_status(s, PROTO_MAX_CHANNELS, buf, sizeof(buf));
    TEST_ASSERT_GREATER_THAN_INT(0, n);
    TEST_ASSERT_EQUAL_INT((int)strlen(buf), n);
    TEST_ASSERT_EQUAL_STRING(" IN16=OFF\r\n", buf + n - 11);
}

static void test_status_rejects_bad_count(void)
{
    char buf[PROTO_RESPONSE_MAX];
    bool s[PROTO_MAX_CHANNELS + 1] = { false };
    TEST_ASSERT_EQUAL_INT(-1, proto_format_status(s, 0, buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_INT(-1, proto_format_status(s, PROTO_MAX_CHANNELS + 1, buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_INT(-1, proto_format_status(NULL, 2, buf, sizeof(buf)));
}

static void test_formatters_report_small_buffer(void)
{
    char buf[8];
    bool s[2] = { true, true };
    proto_cmd_t off = parse("IN1_OFF");
    proto_cmd_t all = parse("ALL_OFF");
    TEST_ASSERT_EQUAL_INT(-1, proto_format_status(s, 2, buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_INT(-1, proto_format_hello("1.0.0", buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_INT(-1, proto_format_result(&off, false, buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_INT(-1, proto_format_result(&all, false, buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_INT(-1, proto_format_status(s, 2, NULL, 0));
    TEST_ASSERT_EQUAL_INT(-1, proto_format_hello(NULL, buf, sizeof(buf)));
    /* Exactly one byte short of "STATUS IN1=ON IN2=ON\r\n" + NUL. */
    char tight[22];
    TEST_ASSERT_EQUAL_INT(-1, proto_format_status(s, 2, tight, sizeof(tight)));
    char fits[23];
    TEST_ASSERT_EQUAL_INT(22, proto_format_status(s, 2, fits, sizeof(fits)));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_parses_fixed_commands);
    RUN_TEST(test_parses_original_relay_commands);
    RUN_TEST(test_parses_every_channel_up_to_max);
    RUN_TEST(test_rejects_bad_channel_numbers);
    RUN_TEST(test_is_case_sensitive);
    RUN_TEST(test_rejects_near_misses);
    RUN_TEST(test_embedded_nul_never_matches);
    RUN_TEST(test_ignores_rom_bootloader_text);
    RUN_TEST(test_formats_relay_results_unchanged);
    RUN_TEST(test_formats_all_results);
    RUN_TEST(test_result_rejects_non_relay_commands);
    RUN_TEST(test_formats_hello);
    RUN_TEST(test_hello_fits_longest_version);
    RUN_TEST(test_formats_nack);
    RUN_TEST(test_formats_two_channel_status_unchanged);
    RUN_TEST(test_formats_status_any_count);
    RUN_TEST(test_max_status_fits_response_buffer);
    RUN_TEST(test_status_rejects_bad_count);
    RUN_TEST(test_formatters_report_small_buffer);
    return UNITY_END();
}
