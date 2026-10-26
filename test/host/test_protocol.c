#include <string.h>

#include "protocol.h"
#include "unity.h"

void setUp(void) {}
void tearDown(void) {}

static proto_cmd_t parse(const char *s)
{
    return proto_parse(s, strlen(s));
}

static void test_parses_every_command(void)
{
    TEST_ASSERT_EQUAL(PROTO_CMD_IN1_ON, parse("IN1_ON"));
    TEST_ASSERT_EQUAL(PROTO_CMD_IN1_OFF, parse("IN1_OFF"));
    TEST_ASSERT_EQUAL(PROTO_CMD_IN2_ON, parse("IN2_ON"));
    TEST_ASSERT_EQUAL(PROTO_CMD_IN2_OFF, parse("IN2_OFF"));
    TEST_ASSERT_EQUAL(PROTO_CMD_HELLO, parse("HeyU"));
    TEST_ASSERT_EQUAL(PROTO_CMD_STATUS, parse("STATUS"));
}

static void test_is_case_sensitive(void)
{
    TEST_ASSERT_EQUAL(PROTO_CMD_NONE, parse("in1_on"));
    TEST_ASSERT_EQUAL(PROTO_CMD_NONE, parse("heyu"));
    TEST_ASSERT_EQUAL(PROTO_CMD_NONE, parse("HEYU"));
    TEST_ASSERT_EQUAL(PROTO_CMD_NONE, parse("status"));
}

static void test_rejects_near_misses(void)
{
    TEST_ASSERT_EQUAL(PROTO_CMD_NONE, parse(""));
    TEST_ASSERT_EQUAL(PROTO_CMD_NONE, parse("IN1_O"));
    TEST_ASSERT_EQUAL(PROTO_CMD_NONE, parse("IN1_ONX"));
    TEST_ASSERT_EQUAL(PROTO_CMD_NONE, parse(" IN1_ON"));
    TEST_ASSERT_EQUAL(PROTO_CMD_NONE, parse("IN1_ON "));
    TEST_ASSERT_EQUAL(PROTO_CMD_NONE, parse("IN1_ON\r"));
    TEST_ASSERT_EQUAL(PROTO_CMD_NONE, parse("IN3_ON"));
    TEST_ASSERT_EQUAL(PROTO_CMD_NONE, proto_parse(NULL, 0));
}

static void test_embedded_nul_never_matches(void)
{
    TEST_ASSERT_EQUAL(PROTO_CMD_NONE, proto_parse("IN1_ON\0x", 8));
    TEST_ASSERT_EQUAL(PROTO_CMD_NONE, proto_parse("IN1_ON\0", 7));
}

static void test_ignores_rom_bootloader_text(void)
{
    TEST_ASSERT_EQUAL(PROTO_CMD_NONE, parse("rst:0x1 (POWERON_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)"));
    TEST_ASSERT_EQUAL(PROTO_CMD_NONE, parse("ets Jul 29 2019 12:21:46"));
    TEST_ASSERT_EQUAL(PROTO_CMD_NONE, parse("entry 0x400805e4"));
}

static void test_relay_target_mapping(void)
{
    unsigned ch = 99;
    bool on = false;

    TEST_ASSERT_TRUE(proto_cmd_relay_target(PROTO_CMD_IN1_ON, &ch, &on));
    TEST_ASSERT_EQUAL_UINT(0, ch);
    TEST_ASSERT_TRUE(on);
    TEST_ASSERT_TRUE(proto_cmd_relay_target(PROTO_CMD_IN1_OFF, &ch, &on));
    TEST_ASSERT_EQUAL_UINT(0, ch);
    TEST_ASSERT_FALSE(on);
    TEST_ASSERT_TRUE(proto_cmd_relay_target(PROTO_CMD_IN2_ON, &ch, &on));
    TEST_ASSERT_EQUAL_UINT(1, ch);
    TEST_ASSERT_TRUE(on);
    TEST_ASSERT_TRUE(proto_cmd_relay_target(PROTO_CMD_IN2_OFF, &ch, &on));
    TEST_ASSERT_EQUAL_UINT(1, ch);
    TEST_ASSERT_FALSE(on);
    TEST_ASSERT_FALSE(proto_cmd_relay_target(PROTO_CMD_HELLO, &ch, &on));
    TEST_ASSERT_FALSE(proto_cmd_relay_target(PROTO_CMD_STATUS, &ch, &on));
    TEST_ASSERT_FALSE(proto_cmd_relay_target(PROTO_CMD_NONE, &ch, &on));
}

static void assert_relay_result(proto_cmd_t cmd, bool ok, const char *expected)
{
    char buf[PROTO_RESPONSE_MAX];
    int n = proto_format_relay_result(cmd, ok, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_INT((int)strlen(expected), n);
    TEST_ASSERT_EQUAL_STRING(expected, buf);
}

static void test_formats_relay_results(void)
{
    assert_relay_result(PROTO_CMD_IN1_ON, true, "IN1_ON_OK\r\n");
    assert_relay_result(PROTO_CMD_IN1_ON, false, "IN1_ON_FAIL\r\n");
    assert_relay_result(PROTO_CMD_IN1_OFF, true, "IN1_OFF_OK\r\n");
    assert_relay_result(PROTO_CMD_IN1_OFF, false, "IN1_OFF_FAIL\r\n");
    assert_relay_result(PROTO_CMD_IN2_ON, true, "IN2_ON_OK\r\n");
    assert_relay_result(PROTO_CMD_IN2_ON, false, "IN2_ON_FAIL\r\n");
    assert_relay_result(PROTO_CMD_IN2_OFF, true, "IN2_OFF_OK\r\n");
    assert_relay_result(PROTO_CMD_IN2_OFF, false, "IN2_OFF_FAIL\r\n");
}

static void test_relay_result_rejects_non_relay_command(void)
{
    char buf[PROTO_RESPONSE_MAX];
    TEST_ASSERT_EQUAL_INT(-1, proto_format_relay_result(PROTO_CMD_HELLO, true, buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_INT(-1, proto_format_relay_result(PROTO_CMD_NONE, true, buf, sizeof(buf)));
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

static void test_formats_status(void)
{
    char buf[PROTO_RESPONSE_MAX];
    proto_format_status(false, false, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("STATUS IN1=OFF IN2=OFF\r\n", buf);
    proto_format_status(true, false, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("STATUS IN1=ON IN2=OFF\r\n", buf);
    proto_format_status(false, true, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("STATUS IN1=OFF IN2=ON\r\n", buf);
    proto_format_status(true, true, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("STATUS IN1=ON IN2=ON\r\n", buf);
}

static void test_formatters_report_small_buffer(void)
{
    char buf[8];
    TEST_ASSERT_EQUAL_INT(-1, proto_format_status(true, true, buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_INT(-1, proto_format_hello("1.0.0", buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_INT(-1, proto_format_relay_result(PROTO_CMD_IN1_OFF, false, buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_INT(-1, proto_format_status(true, true, NULL, 0));
    TEST_ASSERT_EQUAL_INT(-1, proto_format_hello(NULL, buf, sizeof(buf)));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_parses_every_command);
    RUN_TEST(test_is_case_sensitive);
    RUN_TEST(test_rejects_near_misses);
    RUN_TEST(test_embedded_nul_never_matches);
    RUN_TEST(test_ignores_rom_bootloader_text);
    RUN_TEST(test_relay_target_mapping);
    RUN_TEST(test_formats_relay_results);
    RUN_TEST(test_relay_result_rejects_non_relay_command);
    RUN_TEST(test_formats_hello);
    RUN_TEST(test_hello_fits_longest_version);
    RUN_TEST(test_formats_status);
    RUN_TEST(test_formatters_report_small_buffer);
    return UNITY_END();
}
