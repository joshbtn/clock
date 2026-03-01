#include "unity.h"
#include "datetime.h"

// ============================================================================
// TEST: Day of Week Calculations (Zeller's Congruence)
// ============================================================================

void test_getDayOfWeek_knownDates(void) {
  // Known dates with verified day-of-week
  // March 1, 2026 is a Saturday (6)
  TEST_ASSERT_EQUAL(6, getDayOfWeek(2026, 3, 1));
  
  // January 1, 2000 was a Saturday (6)
  TEST_ASSERT_EQUAL(6, getDayOfWeek(2000, 1, 1));
  
  // July 4, 2023 was a Tuesday (2)
  TEST_ASSERT_EQUAL(2, getDayOfWeek(2023, 7, 4));
  
  // December 25, 2020 was a Friday (5)
  TEST_ASSERT_EQUAL(5, getDayOfWeek(2020, 12, 25));
}

void test_getDayOfWeek_sundays(void) {
  // Verify that known Sundays return 0
  // March 2, 2026 is a Sunday (0)
  TEST_ASSERT_EQUAL(0, getDayOfWeek(2026, 3, 2));
  
  // October 4, 2026 is a Sunday (0)
  TEST_ASSERT_EQUAL(0, getDayOfWeek(2026, 10, 4));
}

void test_getDayOfWeek_leapYears(void) {
  // Leap year: 2020
  TEST_ASSERT_EQUAL(0, getDayOfWeek(2020, 2, 29));  // Sunday
  
  // Non-leap year: 2021
  // February 28, 2021 is Sunday (0)
  TEST_ASSERT_EQUAL(0, getDayOfWeek(2021, 2, 28));
}

// ============================================================================
// TEST: Nth Sunday Finder
// ============================================================================

void test_getNthSunday_firstSunday(void) {
  // March 2026: 1st Sunday is March 2
  TEST_ASSERT_EQUAL(2, getNthSunday(2026, 3, 1));
  
  // January 2026: 1st Sunday is January 4
  TEST_ASSERT_EQUAL(4, getNthSunday(2026, 1, 1));
  
  // November 2026: 1st Sunday is November 1
  TEST_ASSERT_EQUAL(1, getNthSunday(2026, 11, 1));
}

void test_getNthSunday_secondSunday(void) {
  // March 2026: 2nd Sunday is March 9
  TEST_ASSERT_EQUAL(9, getNthSunday(2026, 3, 2));
  
  // November 2026: 2nd Sunday is November 8
  TEST_ASSERT_EQUAL(8, getNthSunday(2026, 11, 2));
}

void test_getNthSunday_thirdSunday(void) {
  // October 2026: 3rd Sunday is October 18
  TEST_ASSERT_EQUAL(18, getNthSunday(2026, 10, 3));
  
  // February 2026: 3rd Sunday is February 15
  TEST_ASSERT_EQUAL(15, getNthSunday(2026, 2, 3));
}

void test_getNthSunday_lastSunday(void) {
  // March 2026: Last Sunday is March 29
  TEST_ASSERT_EQUAL(29, getNthSunday(2026, 3, -1));
  
  // October 2026: Last Sunday is October 25
  TEST_ASSERT_EQUAL(25, getNthSunday(2026, 10, -1));
  
  // November 2026: Last Sunday is November 29
  TEST_ASSERT_EQUAL(29, getNthSunday(2026, 11, -1));
  
  // February 2026 (non-leap): Last Sunday is February 22
  TEST_ASSERT_EQUAL(22, getNthSunday(2026, 2, -1));
}

// ============================================================================
// TEST: format12Hour Conversion
// ============================================================================

void test_format12Hour_midnight(void) {
  // Midnight (0:xx) becomes 12 in 12-hour format
  TEST_ASSERT_EQUAL(12, format12Hour(0));
}

void test_format12Hour_morning(void) {
  // 1 AM - 11 AM stay the same
  TEST_ASSERT_EQUAL(1, format12Hour(1));
  TEST_ASSERT_EQUAL(6, format12Hour(6));
  TEST_ASSERT_EQUAL(11, format12Hour(11));
}

void test_format12Hour_noon(void) {
  // Noon (12:xx) stays 12
  TEST_ASSERT_EQUAL(12, format12Hour(12));
}

void test_format12Hour_afternoon(void) {
  // 1 PM - 11 PM (13:00 - 23:00) become 1-11
  TEST_ASSERT_EQUAL(1, format12Hour(13));
  TEST_ASSERT_EQUAL(6, format12Hour(18));
  TEST_ASSERT_EQUAL(11, format12Hour(23));
}

// ============================================================================
// TEST: USA/Canada DST
// ============================================================================

void test_isDSTActive_USA_Canada_beforeDST(void) {
  // February: no DST
  TEST_ASSERT_FALSE(isDSTActive_USA_Canada(2026, 2, 1));
  TEST_ASSERT_FALSE(isDSTActive_USA_Canada(2026, 2, 28));
}

void test_isDSTActive_USA_Canada_marchTransition(void) {
  // March 2026: 2nd Sunday is March 9
  // Before 2nd Sunday: no DST
  TEST_ASSERT_FALSE(isDSTActive_USA_Canada(2026, 3, 8));
  TEST_ASSERT_FALSE(isDSTActive_USA_Canada(2026, 3, 1));
  
  // From 2nd Sunday onward: DST active
  TEST_ASSERT_TRUE(isDSTActive_USA_Canada(2026, 3, 9));
  TEST_ASSERT_TRUE(isDSTActive_USA_Canada(2026, 3, 10));
  TEST_ASSERT_TRUE(isDSTActive_USA_Canada(2026, 3, 31));
}

void test_isDSTActive_USA_Canada_summer(void) {
  // April through October: DST active
  TEST_ASSERT_TRUE(isDSTActive_USA_Canada(2026, 4, 1));
  TEST_ASSERT_TRUE(isDSTActive_USA_Canada(2026, 6, 15));
  TEST_ASSERT_TRUE(isDSTActive_USA_Canada(2026, 10, 31));
}

void test_isDSTActive_USA_Canada_novemberTransition(void) {
  // November 2026: 1st Sunday is November 1
  // Before 1st Sunday: DST active
  TEST_ASSERT_TRUE(isDSTActive_USA_Canada(2026, 11, 0));  // (0 won't occur but tests boundary)
  
  // From 1st Sunday onward: no DST
  TEST_ASSERT_FALSE(isDSTActive_USA_Canada(2026, 11, 1));
  TEST_ASSERT_FALSE(isDSTActive_USA_Canada(2026, 11, 30));
}

void test_isDSTActive_USA_Canada_december(void) {
  // December: no DST
  TEST_ASSERT_FALSE(isDSTActive_USA_Canada(2026, 12, 1));
  TEST_ASSERT_FALSE(isDSTActive_USA_Canada(2026, 12, 31));
}

// ============================================================================
// TEST: UK/EU DST
// ============================================================================

void test_isDSTActive_UK_beforeDST(void) {
  // February: no DST
  TEST_ASSERT_FALSE(isDSTActive_UK(2026, 2, 15));
}

void test_isDSTActive_UK_marchTransition(void) {
  // March 2026: Last Sunday is March 29
  // Before last Sunday: no DST
  TEST_ASSERT_FALSE(isDSTActive_UK(2026, 3, 28));
  TEST_ASSERT_FALSE(isDSTActive_UK(2026, 3, 1));
  
  // From last Sunday onward: DST active
  TEST_ASSERT_TRUE(isDSTActive_UK(2026, 3, 29));
  TEST_ASSERT_TRUE(isDSTActive_UK(2026, 3, 31));
}

void test_isDSTActive_UK_summer(void) {
  // April through September: DST active
  TEST_ASSERT_TRUE(isDSTActive_UK(2026, 4, 1));
  TEST_ASSERT_TRUE(isDSTActive_UK(2026, 7, 15));
  TEST_ASSERT_TRUE(isDSTActive_UK(2026, 9, 30));
}

void test_isDSTActive_UK_octoberTransition(void) {
  // October 2026: Last Sunday is October 25
  // Before last Sunday: DST active
  TEST_ASSERT_TRUE(isDSTActive_UK(2026, 10, 24));
  
  // From last Sunday onward: no DST
  TEST_ASSERT_FALSE(isDSTActive_UK(2026, 10, 25));
  TEST_ASSERT_FALSE(isDSTActive_UK(2026, 10, 31));
}

void test_isDSTActive_UK_november(void) {
  // November: no DST
  TEST_ASSERT_FALSE(isDSTActive_UK(2026, 11, 15));
}

// ============================================================================
// TEST: Australia DST
// ============================================================================

void test_isDSTActive_Australia_octoberTransition(void) {
  // October 2026: 1st Sunday is October 4
  // Before 1st Sunday: no DST
  TEST_ASSERT_FALSE(isDSTActive_Australia(2026, 10, 3));
  TEST_ASSERT_FALSE(isDSTActive_Australia(2026, 10, 1));
  
  // From 1st Sunday onward: DST active
  TEST_ASSERT_TRUE(isDSTActive_Australia(2026, 10, 4));
  TEST_ASSERT_TRUE(isDSTActive_Australia(2026, 10, 31));
}

void test_isDSTActive_Australia_summer(void) {
  // November through March: DST active
  TEST_ASSERT_TRUE(isDSTActive_Australia(2026, 11, 1));
  TEST_ASSERT_TRUE(isDSTActive_Australia(2026, 1, 15));
  TEST_ASSERT_TRUE(isDSTActive_Australia(2026, 3, 31));
}

void test_isDSTActive_Australia_aprilTransition(void) {
  // April 2026: 1st Sunday is April 5
  // Before 1st Sunday: DST active
  TEST_ASSERT_TRUE(isDSTActive_Australia(2026, 4, 4));
  
  // From 1st Sunday onward: no DST
  TEST_ASSERT_FALSE(isDSTActive_Australia(2026, 4, 5));
  TEST_ASSERT_FALSE(isDSTActive_Australia(2026, 4, 30));
}

void test_isDSTActive_Australia_winter(void) {
  // May through September: no DST
  TEST_ASSERT_FALSE(isDSTActive_Australia(2026, 5, 15));
  TEST_ASSERT_FALSE(isDSTActive_Australia(2026, 7, 15));
  TEST_ASSERT_FALSE(isDSTActive_Australia(2026, 9, 30));
}

// ============================================================================
// TEST: New Zealand DST
// ============================================================================

void test_isDSTActive_NewZealand_septemberTransition(void) {
  // September 2026: Last Sunday is September 27
  // Before last Sunday: no DST
  TEST_ASSERT_FALSE(isDSTActive_NewZealand(2026, 9, 26));
  
  // From last Sunday onward: DST active
  TEST_ASSERT_TRUE(isDSTActive_NewZealand(2026, 9, 27));
  TEST_ASSERT_TRUE(isDSTActive_NewZealand(2026, 9, 30));
}

void test_isDSTActive_NewZealand_summer(void) {
  // October through March: DST active
  TEST_ASSERT_TRUE(isDSTActive_NewZealand(2026, 10, 1));
  TEST_ASSERT_TRUE(isDSTActive_NewZealand(2026, 1, 15));
  TEST_ASSERT_TRUE(isDSTActive_NewZealand(2026, 3, 31));
}

void test_isDSTActive_NewZealand_aprilTransition(void) {
  // April 2026: 1st Sunday is April 5
  // Before 1st Sunday: DST active
  TEST_ASSERT_TRUE(isDSTActive_NewZealand(2026, 4, 4));
  
  // From 1st Sunday onward: no DST
  TEST_ASSERT_FALSE(isDSTActive_NewZealand(2026, 4, 5));
  TEST_ASSERT_FALSE(isDSTActive_NewZealand(2026, 4, 30));
}

void test_isDSTActive_NewZealand_winter(void) {
  // May through August: no DST
  TEST_ASSERT_FALSE(isDSTActive_NewZealand(2026, 5, 15));
  TEST_ASSERT_FALSE(isDSTActive_NewZealand(2026, 8, 15));
}

// ============================================================================
// TEST: Brazil DST
// ============================================================================

void test_isDSTActive_Brazil_octoberTransition(void) {
  // October 2026: 3rd Sunday is October 18
  // Before 3rd Sunday: no DST
  TEST_ASSERT_FALSE(isDSTActive_Brazil(2026, 10, 17));
  TEST_ASSERT_FALSE(isDSTActive_Brazil(2026, 10, 1));
  
  // From 3rd Sunday onward: DST active
  TEST_ASSERT_TRUE(isDSTActive_Brazil(2026, 10, 18));
  TEST_ASSERT_TRUE(isDSTActive_Brazil(2026, 10, 31));
}

void test_isDSTActive_Brazil_summer(void) {
  // November and December and January: DST active
  TEST_ASSERT_TRUE(isDSTActive_Brazil(2026, 11, 1));
  TEST_ASSERT_TRUE(isDSTActive_Brazil(2026, 12, 15));
  TEST_ASSERT_TRUE(isDSTActive_Brazil(2026, 1, 15));
}

void test_isDSTActive_Brazil_februaryTransition(void) {
  // February 2026: 3rd Sunday is February 15
  // Before 3rd Sunday: DST active
  TEST_ASSERT_TRUE(isDSTActive_Brazil(2026, 2, 14));
  
  // From 3rd Sunday onward: no DST
  TEST_ASSERT_FALSE(isDSTActive_Brazil(2026, 2, 15));
  TEST_ASSERT_FALSE(isDSTActive_Brazil(2026, 2, 28));
}

void test_isDSTActive_Brazil_winter(void) {
  // March through September: no DST
  TEST_ASSERT_FALSE(isDSTActive_Brazil(2026, 3, 15));
  TEST_ASSERT_FALSE(isDSTActive_Brazil(2026, 6, 15));
  TEST_ASSERT_FALSE(isDSTActive_Brazil(2026, 9, 30));
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
  
  // Day of week tests
  RUN_TEST(test_getDayOfWeek_knownDates);
  RUN_TEST(test_getDayOfWeek_sundays);
  RUN_TEST(test_getDayOfWeek_leapYears);
  
  // Nth Sunday tests
  RUN_TEST(test_getNthSunday_firstSunday);
  RUN_TEST(test_getNthSunday_secondSunday);
  RUN_TEST(test_getNthSunday_thirdSunday);
  RUN_TEST(test_getNthSunday_lastSunday);
  
  // 12-hour format tests
  RUN_TEST(test_format12Hour_midnight);
  RUN_TEST(test_format12Hour_morning);
  RUN_TEST(test_format12Hour_noon);
  RUN_TEST(test_format12Hour_afternoon);
  
  // USA/Canada DST tests
  RUN_TEST(test_isDSTActive_USA_Canada_beforeDST);
  RUN_TEST(test_isDSTActive_USA_Canada_marchTransition);
  RUN_TEST(test_isDSTActive_USA_Canada_summer);
  RUN_TEST(test_isDSTActive_USA_Canada_novemberTransition);
  RUN_TEST(test_isDSTActive_USA_Canada_december);
  
  // UK/EU DST tests
  RUN_TEST(test_isDSTActive_UK_beforeDST);
  RUN_TEST(test_isDSTActive_UK_marchTransition);
  RUN_TEST(test_isDSTActive_UK_summer);
  RUN_TEST(test_isDSTActive_UK_octoberTransition);
  RUN_TEST(test_isDSTActive_UK_november);
  
  // Australia DST tests
  RUN_TEST(test_isDSTActive_Australia_octoberTransition);
  RUN_TEST(test_isDSTActive_Australia_summer);
  RUN_TEST(test_isDSTActive_Australia_aprilTransition);
  RUN_TEST(test_isDSTActive_Australia_winter);
  
  // New Zealand DST tests
  RUN_TEST(test_isDSTActive_NewZealand_septemberTransition);
  RUN_TEST(test_isDSTActive_NewZealand_summer);
  RUN_TEST(test_isDSTActive_NewZealand_aprilTransition);
  RUN_TEST(test_isDSTActive_NewZealand_winter);
  
  // Brazil DST tests
  RUN_TEST(test_isDSTActive_Brazil_octoberTransition);
  RUN_TEST(test_isDSTActive_Brazil_summer);
  RUN_TEST(test_isDSTActive_Brazil_februaryTransition);
  RUN_TEST(test_isDSTActive_Brazil_winter);
  
  UNITY_END();
}
