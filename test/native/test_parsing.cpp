#include "unity.h"
#include <stdio.h>
#include <string.h>

// ============================================================================
// HELPER FUNCTIONS FOR TESTING SERIAL PARSING
// ============================================================================

// Extract and validate time values from "T<h>,<m>,<s>" format
// Returns true if valid, fills in h, m, s
bool parseTimeCommand(const char* cmd, int* h, int* m, int* s) {
  if (!cmd || cmd[0] != 'T') return false;
  
  if (sscanf(cmd + 1, "%d,%d,%d", h, m, s) == 3 &&
      *h >= 0 && *h <= 23 && *m >= 0 && *m <= 59 && *s >= 0 && *s <= 59) {
    return true;
  }
  return false;
}

// Extract and validate date values from "D<m>,<d>,<y>" format
bool parseDateCommand(const char* cmd, int* m, int* d, int* y) {
  if (!cmd || cmd[0] != 'D') return false;
  
  if (sscanf(cmd + 1, "%d,%d,%d", m, d, y) == 3 &&
      *m >= 1 && *m <= 12 && *d >= 1 && *d <= 31 && *y >= 2026 && *y <= 2035) {
    return true;
  }
  return false;
}

// Extract and validate brightness from "B<0-7>" format
bool parseBrightnessCommand(const char* cmd, int* brightness) {
  if (!cmd || cmd[0] != 'B') return false;
  
  *brightness = atoi(cmd + 1);
  return *brightness >= 0 && *brightness <= 7;
}

// Extract and validate timezone from "Z<id>" format
bool parseTimezoneCommand(const char* cmd, int* tz_id) {
  if (!cmd || cmd[0] != 'Z') return false;
  
  *tz_id = atoi(cmd + 1);
  return *tz_id >= 0 && *tz_id <= 20;  // 21 timezones (0-20)
}

// Extract and validate format from "F<0|1>" format
bool parseFormatCommand(const char* cmd, int* format) {
  if (!cmd || cmd[0] != 'F') return false;
  
  if (sscanf(cmd + 1, "%d", format) == 1 && (*format == 0 || *format == 1)) {
    return true;
  }
  return false;
}

// ============================================================================
// TEST: Time Command Parsing (T<h>,<m>,<s>)
// ============================================================================

void test_parseTime_validTimes(void) {
  int h, m, s;
  
  // Valid time: 12:34:56
  TEST_ASSERT_TRUE(parseTimeCommand("T12,34,56", &h, &m, &s));
  TEST_ASSERT_EQUAL(12, h);
  TEST_ASSERT_EQUAL(34, m);
  TEST_ASSERT_EQUAL(56, s);
  
  // Midnight: 00:00:00
  TEST_ASSERT_TRUE(parseTimeCommand("T0,0,0", &h, &m, &s));
  TEST_ASSERT_EQUAL(0, h);
  TEST_ASSERT_EQUAL(0, m);
  TEST_ASSERT_EQUAL(0, s);
  
  // End of day: 23:59:59
  TEST_ASSERT_TRUE(parseTimeCommand("T23,59,59", &h, &m, &s));
  TEST_ASSERT_EQUAL(23, h);
  TEST_ASSERT_EQUAL(59, m);
  TEST_ASSERT_EQUAL(59, s);
}

void test_parseTime_invalidHours(void) {
  int h, m, s;
  
  // Hour > 23
  TEST_ASSERT_FALSE(parseTimeCommand("T24,0,0", &h, &m, &s));
  
  // Negative hour
  TEST_ASSERT_FALSE(parseTimeCommand("T-1,0,0", &h, &m, &s));
}

void test_parseTime_invalidMinutes(void) {
  int h, m, s;
  
  // Minutes > 59
  TEST_ASSERT_FALSE(parseTimeCommand("T12,60,30", &h, &m, &s));
  
  // Negative minutes
  TEST_ASSERT_FALSE(parseTimeCommand("T12,-1,30", &h, &m, &s));
}

void test_parseTime_invalidSeconds(void) {
  int h, m, s;
  
  // Seconds > 59
  TEST_ASSERT_FALSE(parseTimeCommand("T12,30,60", &h, &m, &s));
  
  // Negative seconds
  TEST_ASSERT_FALSE(parseTimeCommand("T12,30,-1", &h, &m, &s));
}

void test_parseTime_malformedInput(void) {
  int h, m, s;
  
  // Wrong prefix
  TEST_ASSERT_FALSE(parseTimeCommand("X12,34,56", &h, &m, &s));
  
  // Missing separator
  TEST_ASSERT_FALSE(parseTimeCommand("T123456", &h, &m, &s));
  
  // Wrong number of args
  TEST_ASSERT_FALSE(parseTimeCommand("T12,34", &h, &m, &s));
  
  // Empty string
  TEST_ASSERT_FALSE(parseTimeCommand("", &h, &m, &s));
  
  // Null pointer
  TEST_ASSERT_FALSE(parseTimeCommand(NULL, &h, &m, &s));
}

// ============================================================================
// TEST: Date Command Parsing (D<m>,<d>,<y>)
// ============================================================================

void test_parseDate_validDates(void) {
  int m, d, y;
  
  // Valid date: March 15, 2026
  TEST_ASSERT_TRUE(parseDateCommand("D3,15,2026", &m, &d, &y));
  TEST_ASSERT_EQUAL(3, m);
  TEST_ASSERT_EQUAL(15, d);
  TEST_ASSERT_EQUAL(2026, y);
  
  // January 1, 2026
  TEST_ASSERT_TRUE(parseDateCommand("D1,1,2026", &m, &d, &y));
  TEST_ASSERT_EQUAL(1, m);
  TEST_ASSERT_EQUAL(1, d);
  TEST_ASSERT_EQUAL(2026, y);
  
  // December 31, 2035
  TEST_ASSERT_TRUE(parseDateCommand("D12,31,2035", &m, &d, &y));
  TEST_ASSERT_EQUAL(12, m);
  TEST_ASSERT_EQUAL(31, d);
  TEST_ASSERT_EQUAL(2035, y);
}

void test_parseDate_invalidMonths(void) {
  int m, d, y;
  
  // Month > 12
  TEST_ASSERT_FALSE(parseDateCommand("D13,1,2026", &m, &d, &y));
  
  // Month < 1
  TEST_ASSERT_FALSE(parseDateCommand("D0,1,2026", &m, &d, &y));
}

void test_parseDate_invalidDays(void) {
  int m, d, y;
  
  // Day > 31
  TEST_ASSERT_FALSE(parseDateCommand("D3,32,2026", &m, &d, &y));
  
  // Day < 1
  TEST_ASSERT_FALSE(parseDateCommand("D3,0,2026", &m, &d, &y));
}

void test_parseDate_invalidYears(void) {
  int m, d, y;
  
  // Year < 2026
  TEST_ASSERT_FALSE(parseDateCommand("D3,15,2025", &m, &d, &y));
  
  // Year > 2035
  TEST_ASSERT_FALSE(parseDateCommand("D3,15,2036", &m, &d, &y));
}

void test_parseDate_malformedInput(void) {
  int m, d, y;
  
  // Wrong prefix
  TEST_ASSERT_FALSE(parseDateCommand("X3,15,2026", &m, &d, &y));
  
  // Missing separator
  TEST_ASSERT_FALSE(parseDateCommand("D31526", &m, &d, &y));
  
  // Empty string
  TEST_ASSERT_FALSE(parseDateCommand("", &m, &d, &y));
}

// ============================================================================
// TEST: Brightness Command Parsing (B<0-7>)
// ============================================================================

void test_parseBrightness_validBrightness(void) {
  int brightness;
  
  // Min brightness
  TEST_ASSERT_TRUE(parseBrightnessCommand("B0", &brightness));
  TEST_ASSERT_EQUAL(0, brightness);
  
  // Mid brightness
  TEST_ASSERT_TRUE(parseBrightnessCommand("B5", &brightness));
  TEST_ASSERT_EQUAL(5, brightness);
  
  // Max brightness
  TEST_ASSERT_TRUE(parseBrightnessCommand("B7", &brightness));
  TEST_ASSERT_EQUAL(7, brightness);
}

void test_parseBrightness_invalidBrightness(void) {
  int brightness;
  
  // Brightness > 7
  TEST_ASSERT_FALSE(parseBrightnessCommand("B8", &brightness));
  
  // Brightness < 0
  TEST_ASSERT_FALSE(parseBrightnessCommand("B-1", &brightness));
}

void test_parseBrightness_malformedInput(void) {
  int brightness;
  
  // Wrong prefix
  TEST_ASSERT_FALSE(parseBrightnessCommand("X5", &brightness));
  
  // Empty string
  TEST_ASSERT_FALSE(parseBrightnessCommand("", &brightness));
}

// ============================================================================
// TEST: Timezone Command Parsing (Z<id>)
// ============================================================================

void test_parseTimezone_validTimezones(void) {
  int tz;
  
  // UTC
  TEST_ASSERT_TRUE(parseTimezoneCommand("Z0", &tz));
  TEST_ASSERT_EQUAL(0, tz);
  
  // USA Eastern
  TEST_ASSERT_TRUE(parseTimezoneCommand("Z1", &tz));
  TEST_ASSERT_EQUAL(1, tz);
  
  // Max valid timezone
  TEST_ASSERT_TRUE(parseTimezoneCommand("Z20", &tz));
  TEST_ASSERT_EQUAL(20, tz);
}

void test_parseTimezone_invalidTimezones(void) {
  int tz;
  
  // Timezone > 20
  TEST_ASSERT_FALSE(parseTimezoneCommand("Z21", &tz));
  
  // Negative timezone
  TEST_ASSERT_FALSE(parseTimezoneCommand("Z-1", &tz));
}

void test_parseTimezone_malformedInput(void) {
  int tz;
  
  // Wrong prefix
  TEST_ASSERT_FALSE(parseTimezoneCommand("X0", &tz));
  
  // Empty string
  TEST_ASSERT_FALSE(parseTimezoneCommand("", &tz));
}

// ============================================================================
// TEST: Format Command Parsing (F<0|1>)
// ============================================================================

void test_parseFormat_valid24Hour(void) {
  int format;
  
  // 24-hour format
  TEST_ASSERT_TRUE(parseFormatCommand("F0", &format));
  TEST_ASSERT_EQUAL(0, format);
}

void test_parseFormat_valid12Hour(void) {
  int format;
  
  // 12-hour format
  TEST_ASSERT_TRUE(parseFormatCommand("F1", &format));
  TEST_ASSERT_EQUAL(1, format);
}

void test_parseFormat_invalidFormat(void) {
  int format;
  
  // Invalid format value
  TEST_ASSERT_FALSE(parseFormatCommand("F2", &format));
  TEST_ASSERT_FALSE(parseFormatCommand("F-1", &format));
}

void test_parseFormat_malformedInput(void) {
  int format;
  
  // Wrong prefix
  TEST_ASSERT_FALSE(parseFormatCommand("X0", &format));
  
  // Empty string
  TEST_ASSERT_FALSE(parseFormatCommand("", &format));
}

// ============================================================================
// TEST: Edge Cases and Buffer Handling
// ============================================================================

void test_parseTime_leadingZeros(void) {
  int h, m, s;
  
  // Time with leading zeros
  TEST_ASSERT_TRUE(parseTimeCommand("T01,02,03", &h, &m, &s));
  TEST_ASSERT_EQUAL(1, h);
  TEST_ASSERT_EQUAL(2, m);
  TEST_ASSERT_EQUAL(3, s);
}

void test_parseDate_leadingZeros(void) {
  int m, d, y;
  
  // Date with leading zeros
  TEST_ASSERT_TRUE(parseDateCommand("D03,05,2026", &m, &d, &y));
  TEST_ASSERT_EQUAL(3, m);
  TEST_ASSERT_EQUAL(5, d);
  TEST_ASSERT_EQUAL(2026, y);
}

void test_parseTime_extraWhitespace(void) {
  int h, m, s;
  
  // sscanf should handle space after commas
  TEST_ASSERT_TRUE(parseTimeCommand("T12, 34, 56", &h, &m, &s));
  TEST_ASSERT_EQUAL(12, h);
  TEST_ASSERT_EQUAL(34, m);
  TEST_ASSERT_EQUAL(56, s);
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

void setUp(void) {
  // Run before each test
}

void tearDown(void) {
  // Run after each test
}

void main(void) {
  UNITY_BEGIN();
  
  // Time parsing tests
  RUN_TEST(test_parseTime_validTimes);
  RUN_TEST(test_parseTime_invalidHours);
  RUN_TEST(test_parseTime_invalidMinutes);
  RUN_TEST(test_parseTime_invalidSeconds);
  RUN_TEST(test_parseTime_malformedInput);
  RUN_TEST(test_parseTime_leadingZeros);
  RUN_TEST(test_parseTime_extraWhitespace);
  
  // Date parsing tests
  RUN_TEST(test_parseDate_validDates);
  RUN_TEST(test_parseDate_invalidMonths);
  RUN_TEST(test_parseDate_invalidDays);
  RUN_TEST(test_parseDate_invalidYears);
  RUN_TEST(test_parseDate_malformedInput);
  RUN_TEST(test_parseDate_leadingZeros);
  
  // Brightness parsing tests
  RUN_TEST(test_parseBrightness_validBrightness);
  RUN_TEST(test_parseBrightness_invalidBrightness);
  RUN_TEST(test_parseBrightness_malformedInput);
  
  // Timezone parsing tests
  RUN_TEST(test_parseTimezone_validTimezones);
  RUN_TEST(test_parseTimezone_invalidTimezones);
  RUN_TEST(test_parseTimezone_malformedInput);
  
  // Format parsing tests
  RUN_TEST(test_parseFormat_valid24Hour);
  RUN_TEST(test_parseFormat_valid12Hour);
  RUN_TEST(test_parseFormat_invalidFormat);
  RUN_TEST(test_parseFormat_malformedInput);
  
  UNITY_END();
}
