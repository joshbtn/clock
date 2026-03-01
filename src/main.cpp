#include <Wire.h>
#include <EEPROM.h>
#include <RTClib.h>
#include <TM1637Display.h>

#define CLK_PIN 3
#define DIO_PIN 4

// EEPROM Address Map (simplified, no DST tables)
#define ADDR_BRIGHTNESS   0x00  // 1 byte, 0-7
#define ADDR_FORMAT_12H   0x01  // 1 byte, 0=24h, 1=12h
#define ADDR_TZ_ID        0x02  // 1 byte, timezone ID (0-11)

TM1637Display display(CLK_PIN, DIO_PIN);
RTC_DS3231 rtc;

// Timezone definitions with DST rules encoded in ID
struct Timezone {
  uint8_t id;
  int8_t utc_offset_hours;
  const char* name;
  uint8_t dst_rule;  // 0=None, 1=USA/Canada, 2=UK/EU
};

const Timezone timezones[] = {
  {0,   0, "UTC",                 0},  // No DST
  {1,  -5, "USA Eastern",         1},  // EST/EDT
  {2,  -6, "USA Central",         1},  // CST/CDT
  {3,  -7, "USA Mountain",        1},  // MST/MDT
  {4,  -8, "USA Pacific",         1},  // PST/PDT
  {5,  -3, "Canada Atlantic",     1},  // AST/ADT (same rule as USA)
  {6,  -5, "Canada Eastern",      1},  // EST/EDT
  {7,  -6, "Canada Central",      1},  // CST/CDT
  {8,  -7, "Canada Mountain",     0},  // No DST
  {9,  -8, "Canada Pacific",      1},  // PST/PDT
  {10,  0, "UK London",           2},  // GMT/BST
  {11, -7, "Arizona",             0},  // MST year-round, no DST
  {12,-10, "Hawaii",              0},  // HST year-round, no DST
};
const uint8_t NUM_TIMEZONES = sizeof(timezones) / sizeof(timezones[0]);

// Runtime state
DateTime lastDateCheck;
bool dstActive = false;
uint8_t tzId = 0; // Default UTC

// Helper function: convert 24-hour (0-23) to 12-hour (1-12)
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

// Main DST check: dispatches to the appropriate algorithm based on timezone ID
void checkAndApplyDST() {
  DateTime now = rtc.now();
  
  // Get the DST rule type for the current timezone
  uint8_t dstRule = 0;
  for (uint8_t i = 0; i < NUM_TIMEZONES; i++) {
    if (timezones[i].id == tzId) {
      dstRule = timezones[i].dst_rule;
      break;
    }
  }
  
  // Apply the appropriate DST algorithm
  if (dstRule == 1) {
    dstActive = isDSTActive_USA_Canada(now.year(), now.month(), now.day());
  } else if (dstRule == 2) {
    dstActive = isDSTActive_UK(now.year(), now.month(), now.day());
  } else {
    dstActive = false;
  }
}

void updateDisplay() {
  DateTime now = rtc.now();
  uint8_t h = now.hour();
  uint8_t m = now.minute();
  
  uint8_t format = EEPROM.read(ADDR_FORMAT_12H);
  int time;
  
  if (format == 1) {
    // 12-hour format: display as H:MM (no leading zero)
    h = format12Hour(h);
    time = h * 100 + m;
  } else {
    // 24-hour format: display as HH:MM
    time = h * 100 + m;
  }
  
  display.showNumberDecEx(time, 0b01000000, format != 1); // Leading zero in 24-hour mode only
}



// Auto-increment date if 24 hours have passed
static unsigned long lastMillis = 0;
void autoIncrementDate() {
  unsigned long currentMillis = millis();
  
  // Check if ~24 hours (86400000ms) have elapsed
  if (currentMillis - lastMillis >= 86400000UL) {
    lastMillis = currentMillis;
    
    DateTime now = rtc.now();
    
    // Increment date by 1 day
    // RTClib doesn't have a built-in increment, so we use adjust with nextDay logic
    uint16_t day = now.day() + 1;
    uint8_t month = now.month();
    uint16_t year = now.year();
    
    // Simple day overflow (doesn't account for days per month, but good enough)
    uint8_t daysInMonth[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    
    // Check for leap year
    if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
      daysInMonth[2] = 29;
    }
    
    if (day > daysInMonth[month]) {
      day = 1;
      month++;
      if (month > 12) {
        month = 1;
        year++;
      }
    }
    
    rtc.adjust(DateTime(year, month, day, now.hour(), now.minute(), now.second()));
    checkAndApplyDST();
  }
}


void handleSerial() {
  // Read full line into buffer
  static char buf[64];
  static uint8_t pos = 0;

  while (Serial.available()) {
    char c = Serial.read();
    
    // Check for line termination
    if (c == '\n' || c == '\r') {
      if (pos == 0) continue;  // skip empty lines
      buf[pos] = '\0';
      uint8_t cmdLen = pos;
      pos = 0;

      Serial.print("DBG:RX ");
      Serial.println(buf);

      // Parse command from buffer
      if (buf[0] == 'T') {
        // T<hour>,<minute>,<second>
        int h, m, s;
        if (sscanf(buf + 1, "%d,%d,%d", &h, &m, &s) == 3 &&
            h >= 0 && h <= 23 && m >= 0 && m <= 59 && s >= 0 && s <= 59) {
          DateTime now = rtc.now();
          rtc.adjust(DateTime(now.year(), now.month(), now.day(), h, m, s));
          updateDisplay();
          Serial.print("OK:T");
          Serial.print(h); Serial.print(":");
          Serial.print(m); Serial.print(":");
          Serial.println(s);
        } else {
          Serial.println("ERR:T expected h,m,s");
        }
      }
      else if (buf[0] == 'D') {
        // D<month>,<day>,<year>
        int m, d, y;
        if (sscanf(buf + 1, "%d,%d,%d", &m, &d, &y) == 3 &&
            m >= 1 && m <= 12 && d >= 1 && d <= 31 && y >= 2026 && y <= 2035) {
          DateTime now = rtc.now();
          rtc.adjust(DateTime(y, m, d, now.hour(), now.minute(), now.second()));
          lastDateCheck = rtc.now();
          checkAndApplyDST();
          updateDisplay();
          Serial.print("OK:D");
          Serial.print(m); Serial.print("/");
          Serial.print(d); Serial.print("/");
          Serial.println(y);
        } else {
          Serial.println("ERR:D expected m,d,y");
        }
      }
      else if (buf[0] == 'Q' && buf[1] == 'F' && buf[2] == '\0') {
        uint8_t stored = EEPROM.read(ADDR_FORMAT_12H);
        if (stored > 1) stored = 0;
        Serial.print("OK:QF");
        Serial.println(stored);
      }
      else if (buf[0] == 'F') {
        // F<0|1> (0=24h, 1=12h)
        int f;
        if (sscanf(buf + 1, "%d", &f) == 1 && (f == 0 || f == 1)) {
          EEPROM.update(ADDR_FORMAT_12H, f);
          updateDisplay();
          uint8_t stored = EEPROM.read(ADDR_FORMAT_12H);
          DateTime now = rtc.now();
          uint8_t shownHour = (stored == 1) ? format12Hour(now.hour()) : now.hour();
          Serial.print("DBG:F requested=");
          Serial.print(f);
          Serial.print(" stored=");
          Serial.print(stored);
          Serial.print(" rtcHour24=");
          Serial.print(now.hour());
          Serial.print(" shownHour=");
          Serial.println(shownHour);
          Serial.print("OK:F");
          Serial.println(stored);
        } else {
          Serial.println("ERR:F expected 0 or 1");
        }
      }
      else if (buf[0] == 'Z') {
        // Z<tz_id> (0-12)
        // 0=UTC, 1-4=USA, 5-9=Canada, 10=UK, 11=Arizona(no DST), 12=Hawaii(no DST)
        int z = atoi(buf + 1);
        if (z >= 0 && z < NUM_TIMEZONES) {
          EEPROM.update(ADDR_TZ_ID, z);
          tzId = z;
          checkAndApplyDST();
          
          // Find the timezone name
          const char* tzName = "Unknown";
          for (uint8_t i = 0; i < NUM_TIMEZONES; i++) {
            if (timezones[i].id == z) {
              tzName = timezones[i].name;
              break;
            }
          }
          
          Serial.print("OK:Z");
          Serial.println(z);
          Serial.print("DBG:TZ ");
          Serial.println(tzName);
        } else {
          Serial.println("ERR:Z expected 0..12");
        }
      }
      else if (buf[0] == 'B') {
        // B<0-7> (brightness)
        int b = atoi(buf + 1);
        if (b >= 0 && b <= 7) {
          EEPROM.update(ADDR_BRIGHTNESS, b);
          display.setBrightness(b);
          updateDisplay();
          Serial.print("OK:B");
          Serial.println(b);
        } else {
          Serial.println("ERR:B expected 0..7");
        }
      }
      else {
        Serial.print("ERR:UNKNOWN ");
        Serial.println(buf);
      }

      return;
    }
    else if (pos < sizeof(buf) - 1) {
      buf[pos++] = c;
    } else {
      pos = 0;
      Serial.println("ERR:RX overflow");
    }
  }
}


void setup() {
  Serial.begin(9600);
  Wire.begin();
  rtc.begin();

  // Load settings from EEPROM
  uint8_t brightness = EEPROM.read(ADDR_BRIGHTNESS);
  if (brightness > 7) brightness = 5;
  display.setBrightness(brightness);
  
  // Load timezone ID
  uint8_t tzByte = EEPROM.read(ADDR_TZ_ID);
  if (tzByte >= NUM_TIMEZONES) {
    tzId = 0;  // Default UTC
  } else {
    tzId = tzByte;
  }
  
  // Initialize date checking for auto-increment
  lastDateCheck = rtc.now();
  
  // Check initial DST status
  checkAndApplyDST();

  updateDisplay();
}

void loop() {
  handleSerial();
  
  // Auto-increment date every 24 hours
  autoIncrementDate();
  
  updateDisplay();
  delay(500);
}
