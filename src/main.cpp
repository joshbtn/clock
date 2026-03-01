#include <Wire.h>
#include <EEPROM.h>
#include <RTClib.h>
#include <TM1637Display.h>
#include "datetime.h"

#define CLK_PIN 3
#define DIO_PIN 4

// EEPROM Address Map (simplified, no DST tables)
#define ADDR_BRIGHTNESS        0x00  // 1 byte, 0-7
#define ADDR_FORMAT_12H        0x01  // 1 byte, 0=24h, 1=12h
#define ADDR_TZ_ID             0x02  // 1 byte, timezone ID (0-20)
#define ADDR_DST_RULES_VERSION 0x03  // 1 byte, version of DST rules

// DST Rules Version for firmware compatibility checks
#define DST_RULES_VERSION 2

TM1637Display display(CLK_PIN, DIO_PIN);
RTC_DS3231 rtc;

// Timezone definitions with DST rules encoded in ID
struct Timezone {
  uint8_t id;
  int8_t utc_offset_hours;
  const char* name;
  uint8_t dst_rule;  // 0=None, 1=USA/Canada, 2=UK/EU
};

// DST Rule Type Definitions
#define DST_RULE_NONE              0
#define DST_RULE_USA_CANADA        1
#define DST_RULE_UK_EU             2
#define DST_RULE_AUSTRALIA         3
#define DST_RULE_NEW_ZEALAND       4
#define DST_RULE_BRAZIL            5

const Timezone timezones[] = {
  {0,   0, "UTC",                 DST_RULE_NONE},
  {1,  -5, "USA Eastern",         DST_RULE_USA_CANADA},
  {2,  -6, "USA Central",         DST_RULE_USA_CANADA},
  {3,  -7, "USA Mountain",        DST_RULE_USA_CANADA},
  {4,  -8, "USA Pacific",         DST_RULE_USA_CANADA},
  {5,  -3, "Canada Atlantic",     DST_RULE_USA_CANADA},
  {6,  -5, "Canada Eastern",      DST_RULE_USA_CANADA},
  {7,  -6, "Canada Central",      DST_RULE_USA_CANADA},
  {8,  -7, "Canada Mountain",     DST_RULE_NONE},
  {9,  -8, "Canada Pacific",      DST_RULE_USA_CANADA},
  {10,  0, "UK London",           DST_RULE_UK_EU},
  {11, -7, "Arizona",             DST_RULE_NONE},
  {12,-10, "Hawaii",              DST_RULE_NONE},
  {13, -9, "Samoa",               DST_RULE_NONE},
  {14,  1, "EU Central",          DST_RULE_UK_EU},
  {15,  2, "EU Eastern",          DST_RULE_UK_EU},
  {16,-10, "Australia Sydney",    DST_RULE_AUSTRALIA},
  {17, -9, "Australia Adelaide",  DST_RULE_AUSTRALIA},
  {18, -8, "Australia Perth",     DST_RULE_NONE},
  {19, 12, "New Zealand",         DST_RULE_NEW_ZEALAND},
  {20, -3, "Brazil Sao Paulo",    DST_RULE_BRAZIL}
};
const uint8_t NUM_TIMEZONES = sizeof(timezones) / sizeof(timezones[0]);

// Runtime state
DateTime lastDateCheck;
bool dstActive = false;
uint8_t tzId = 0; // Default UTC
// (DateTime functions are now in datetime.h / datetime.cpp)

// Main DST check: dispatches to the appropriate algorithm based on timezone ID
void checkAndApplyDST() {
  DateTime now = rtc.now();
  
  // Get the DST rule type for the current timezone
  uint8_t dstRule = DST_RULE_NONE;
  for (uint8_t i = 0; i < NUM_TIMEZONES; i++) {
    if (timezones[i].id == tzId) {
      dstRule = timezones[i].dst_rule;
      break;
    }
  }
  
  // Apply the appropriate DST algorithm
  switch (dstRule) {
    case DST_RULE_USA_CANADA:
      dstActive = isDSTActive_USA_Canada(now.year(), now.month(), now.day());
      break;
    case DST_RULE_UK_EU:
      dstActive = isDSTActive_UK(now.year(), now.month(), now.day());
      break;
    case DST_RULE_AUSTRALIA:
      dstActive = isDSTActive_Australia(now.year(), now.month(), now.day());
      break;
    case DST_RULE_NEW_ZEALAND:
      dstActive = isDSTActive_NewZealand(now.year(), now.month(), now.day());
      break;
    case DST_RULE_BRAZIL:
      dstActive = isDSTActive_Brazil(now.year(), now.month(), now.day());
      break;
    default:
      dstActive = false;
      break;
  }
}

void updateDisplay() {
  DateTime now = rtc.now();
  uint8_t h = now.hour();
  uint8_t m = now.minute();
  
  uint8_t format = EEPROM.read(ADDR_FORMAT_12H);
  int time;
  uint8_t decimalMask = 0b01000000;  // Middle colon (between digits 1-2)
  
  if (format == 1) {
    // 12-hour format: display as H:MM (no leading zero)
    h = format12Hour(h);
    
    // Light the last dot if PM (12:00 and onward)
    if (now.hour() >= 12) {
      decimalMask |= 0b00010000;  // Add last dot for PM
    }
    
    time = h * 100 + m;
  } else {
    // 24-hour format: display as HH:MM
    time = h * 100 + m;
  }
  
  display.showNumberDecEx(time, decimalMask, format != 1); // Leading zero in 24-hour mode only
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
        // Z<tz_id> (0-20)
        // Timezone ID selector with DST rule dispatch
        int z = atoi(buf + 1);
        if (z >= 0 && z < NUM_TIMEZONES) {
          EEPROM.update(ADDR_TZ_ID, z);
          tzId = z;
          
          // Mark DST rules version
          EEPROM.update(ADDR_DST_RULES_VERSION, DST_RULES_VERSION);
          
          checkAndApplyDST();
          
          // Find the timezone name and DST rule
          const char* tzName = "Unknown";
          uint8_t dstRule = DST_RULE_NONE;
          for (uint8_t i = 0; i < NUM_TIMEZONES; i++) {
            if (timezones[i].id == z) {
              tzName = timezones[i].name;
              dstRule = timezones[i].dst_rule;
              break;
            }
          }
          
          Serial.print("OK:Z");
          Serial.println(z);
          Serial.print("DBG:TZ ");
          Serial.print(tzName);
          Serial.print(" rule=");
          Serial.println(dstRule);
        } else {
          Serial.print("ERR:Z expected 0..");
          Serial.println(NUM_TIMEZONES - 1);
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
  
  Serial.println("DBG:Boot");
  Serial.print("DBG:DST_RULES_VERSION=");
  Serial.println(DST_RULES_VERSION);

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
  
  // Check DST rules version compatibility
  uint8_t storedVersion = EEPROM.read(ADDR_DST_RULES_VERSION);
  if (storedVersion != DST_RULES_VERSION && storedVersion != 0) {
    Serial.print("DBG:RULE_VERSION_MISMATCH stored=");
    Serial.print(storedVersion);
    Serial.print(" current=");
    Serial.println(DST_RULES_VERSION);
  }
  
  // Initialize date checking for auto-increment
  lastDateCheck = rtc.now();
  
  // Check initial DST status
  checkAndApplyDST();
  
  Serial.print("DBG:Timezone=");
  for (uint8_t i = 0; i < NUM_TIMEZONES; i++) {
    if (timezones[i].id == tzId) {
      Serial.println(timezones[i].name);
      break;
    }
  }

  updateDisplay();
}

void loop() {
  handleSerial();
  
  // Auto-increment date every 24 hours
  autoIncrementDate();
  
  updateDisplay();
  delay(500);
}
