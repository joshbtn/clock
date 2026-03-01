#include "datetime.h"

// Convert 24-hour (0-23) to 12-hour (1-12)
uint8_t format12Hour(uint8_t hour24) {
  if (hour24 == 0) return 12;  // Midnight
  if (hour24 > 12) return hour24 - 12;  // PM
  return hour24;  // 1-12 (AM)
}

// Calculate day of week (0=Sunday, 1=Monday, ..., 6=Saturday)
// Uses Zeller's congruence algorithm
uint8_t getDayOfWeek(uint16_t year, uint8_t month, uint8_t day) {
  if (month < 3) {
    month += 12;
    year--;
  }
  uint16_t q = day;
  uint16_t m = month;
  uint16_t k = year % 100;
  uint16_t j = year / 100;
  uint16_t h = (q + ((13 * (m + 1)) / 5) + k + (k / 4) + (j / 4) - (2 * j)) % 7;
  // Zeller returns: 0=Sat, 1=Sun, 2=Mon, ...
  // Convert to: 0=Sun, 1=Mon, ..., 6=Sat
  return (h + 5) % 7;
}

// Get Nth Sunday of a month (n=1 for first, n=-1 for last)
uint8_t getNthSunday(uint16_t year, uint8_t month, int8_t n) {
  uint8_t daysInMonth[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  
  // Check for leap year
  if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
    daysInMonth[2] = 29;
  }
  
  if (n > 0) {
    // Find the Nth Sunday: first check day 1, then add offset
    uint8_t dow = getDayOfWeek(year, month, 1);
    uint8_t firstSunday = 1 + ((7 - dow) % 7);
    return firstSunday + ((n - 1) * 7);
  } else if (n == -1) {
    // Find last Sunday
    uint8_t lastDay = daysInMonth[month];
    uint8_t dow = getDayOfWeek(year, month, lastDay);
    return lastDay - dow;
  }
  
  return 1;
}

// DST for USA/Canada: 2nd Sunday in March to 1st Sunday in November
bool isDSTActive_USA_Canada(uint16_t year, uint8_t month, uint8_t day) {
  // March: DST from 2nd Sunday onward
  if (month == 3) {
    uint8_t secondSunday = getNthSunday(year, 3, 2);
    return day >= secondSunday;
  }
  
  // April-October: DST active
  if (month > 3 && month < 11) {
    return true;
  }
  
  // November: DST until 1st Sunday (not including 1st Sunday)
  if (month == 11) {
    uint8_t firstSunday = getNthSunday(year, 11, 1);
    return day < firstSunday;
  }
  
  // December-February: standard time
  return false;
}

// DST for UK/EU: Last Sunday in March to Last Sunday in October
bool isDSTActive_UK(uint16_t year, uint8_t month, uint8_t day) {
  // March: DST from last Sunday onward
  if (month == 3) {
    uint8_t lastSunday = getNthSunday(year, 3, -1);
    return day >= lastSunday;
  }
  
  // April-September: DST active
  if (month > 3 && month < 10) {
    return true;
  }
  
  // October: DST until last Sunday (not including last Sunday)
  if (month == 10) {
    uint8_t lastSunday = getNthSunday(year, 10, -1);
    return day < lastSunday;
  }
  
  // November-February: standard time
  return false;
}

// DST for Australia (Sydney/Melbourne): 1st Sunday in October to 1st Sunday in April
bool isDSTActive_Australia(uint16_t year, uint8_t month, uint8_t day) {
  // October: DST from 1st Sunday onward
  if (month == 10) {
    uint8_t firstSunday = getNthSunday(year, 10, 1);
    return day >= firstSunday;
  }
  
  // November-March: DST active
  if (month > 10 || month < 4) {
    return true;
  }
  
  // April: DST until 1st Sunday (not including 1st Sunday)
  if (month == 4) {
    uint8_t firstSunday = getNthSunday(year, 4, 1);
    return day < firstSunday;
  }
  
  return false;
}

// DST for New Zealand: Last Sunday in September to 1st Sunday in April
bool isDSTActive_NewZealand(uint16_t year, uint8_t month, uint8_t day) {
  // September: DST from last Sunday onward
  if (month == 9) {
    uint8_t lastSunday = getNthSunday(year, 9, -1);
    return day >= lastSunday;
  }
  
  // October-March: DST active
  if (month > 9 || month < 4) {
    return true;
  }
  
  // April: DST until 1st Sunday (not including 1st Sunday)
  if (month == 4) {
    uint8_t firstSunday = getNthSunday(year, 4, 1);
    return day < firstSunday;
  }
  
  return false;
}

// DST for Brazil: 3rd Sunday in October to 3rd Sunday in February
bool isDSTActive_Brazil(uint16_t year, uint8_t month, uint8_t day) {
  // October: DST from 3rd Sunday onward
  if (month == 10) {
    uint8_t thirdSunday = getNthSunday(year, 10, 3);
    return day >= thirdSunday;
  }
  
  // November-January: DST active
  if (month > 10 && month < 2) {
    return true;
  }
  
  // February: DST until 3rd Sunday (not including 3rd Sunday)
  if (month == 2) {
    uint8_t thirdSunday = getNthSunday(year, 2, 3);
    return day < thirdSunday;
  }
  
  return false;
}
