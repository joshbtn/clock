#pragma once

#include <stdint.h>

// Pure date/time calculation functions - no hardware dependencies
// These functions are testable as they take date values and return computed results

// Convert 24-hour time (0-23) to 12-hour (1-12) format
uint8_t format12Hour(uint8_t hour24);

// Calculate day of week (0=Sunday, 1=Monday, ..., 6=Saturday)
// Uses Zeller's congruence algorithm
uint8_t getDayOfWeek(uint16_t year, uint8_t month, uint8_t day);

// Get Nth Sunday of a month (n=1 for first, n=-1 for last)
uint8_t getNthSunday(uint16_t year, uint8_t month, int8_t n);

// DST algorithms for various regions
// All take year, month, day and return true if DST is active

// USA/Canada: 2nd Sunday in March to 1st Sunday in November
bool isDSTActive_USA_Canada(uint16_t year, uint8_t month, uint8_t day);

// UK/EU: Last Sunday in March to Last Sunday in October
bool isDSTActive_UK(uint16_t year, uint8_t month, uint8_t day);

// Australia (Sydney/Melbourne): 1st Sunday in October to 1st Sunday in April
bool isDSTActive_Australia(uint16_t year, uint8_t month, uint8_t day);

// New Zealand: Last Sunday in September to 1st Sunday in April
bool isDSTActive_NewZealand(uint16_t year, uint8_t month, uint8_t day);

// Brazil: 3rd Sunday in October to 3rd Sunday in February
bool isDSTActive_Brazil(uint16_t year, uint8_t month, uint8_t day);
