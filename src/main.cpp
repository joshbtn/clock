#include <Wire.h>
#include <EEPROM.h>
#include <RTClib.h>
#include <TM1637Display.h>

#define CLK_PIN 3
#define DIO_PIN 4

// EEPROM Address Map
#define ADDR_BRIGHTNESS   0x00  // 1 byte, 0-7
#define ADDR_FORMAT_12H   0x01  // 1 byte, 0=24h, 1=12h
#define ADDR_TZ_OFFSET    0x02  // 1 byte, stored as value+12 (range: -12 to +14, stored 0-26)
#define ADDR_DST_ENABLED  0x03  // 1 byte, 0=off, 1=on
#define ADDR_DST_CHECKSUM 0x05  // 1 byte
#define ADDR_DST_TABLE    0x10  // 40 bytes (10 years × 4 bytes/year)
#define DST_TABLE_SIZE    40

TM1637Display display(CLK_PIN, DIO_PIN);
RTC_DS3231 rtc;

// DST transition data: [year_offset, spring_day, fall_day, reserved]
// Example: {0, 14, 1, 0} means year 2026, spring=Mar 14, fall=Nov 1
struct DSTYear {
  uint8_t year_offset;  // 0-9 (2026-2035)
  uint8_t spring_day;   // 1-31
  uint8_t fall_day;     // 1-31
  uint8_t reserved;     // padding
};

// Runtime state
DateTime lastDateCheck;
bool dstActive = false;
uint8_t tzOffset = 12; // Default 0 (stored as 0+12=12)

// Helper function: convert 24-hour (0-23) to 12-hour (1-12)
uint8_t format12Hour(uint8_t hour24) {
  if (hour24 == 0) return 12;  // Midnight
  if (hour24 > 12) return hour24 - 12;  // PM
  return hour24;  // 1-12 (AM)
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
  
  display.showNumberDecEx(time, 0b01000000, format == 1); // No leading zero in 12-hour mode
}

// Check if today is a DST transition date and update dstActive flag
void checkAndApplyDST() {
  DateTime now = rtc.now();
  
  // Only check if DST is enabled
  if (EEPROM.read(ADDR_DST_ENABLED) != 1) {
    dstActive = false;
    return;
  }
  
  // Find current year's DST transitions from EEPROM table
  uint8_t year_offset = now.year() - 2026;
  if (year_offset < 0 || year_offset >= 10) {
    // Outside supported range, disable DST
    dstActive = false;
    return;
  }
  
  // Read DST entry for this year (4 bytes per year)
  uint8_t spring_day, fall_day;
  EEPROM.get(ADDR_DST_TABLE + (year_offset * 4) + 1, spring_day);
  EEPROM.get(ADDR_DST_TABLE + (year_offset * 4) + 2, fall_day);
  
  // Spring forward: March spring_day → DST active
  // Fall back: November fall_day → DST inactive
  
  bool inSpring = (now.month() > 3) || (now.month() == 3 && now.day() >= spring_day);
  bool beforeFall = (now.month() < 11) || (now.month() == 11 && now.day() < fall_day);
  
  dstActive = inSpring && beforeFall;
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

// Parse incoming DST table from binary format
void parseDSTTable(uint8_t* buffer, uint8_t len) {
  // Expected format: 10 entries × 4 bytes = 40 bytes exactly
  if (len != 40) {
    Serial.println("ERR:DST table must be exactly 40 bytes");
    return;
  }
  
  // Calculate simple checksum (XOR of all bytes)
  uint8_t checksum = 0;
  for (uint8_t i = 0; i < len; i++) {
    checksum ^= buffer[i];
  }
  
  // Write table to EEPROM
  for (uint8_t i = 0; i < len; i++) {
    EEPROM.update(ADDR_DST_TABLE + i, buffer[i]);
  }
  
  // Write checksum
  EEPROM.update(ADDR_DST_CHECKSUM, checksum);
  
  // Re-evaluate DST status
  checkAndApplyDST();
  
  Serial.print("OK:LOAD");
  Serial.println(checksum);
}

void handleSerial() {
  // Read full line into buffer
  static char buf[64];
  static uint8_t pos = 0;
  static uint8_t binaryBuffer[64];  // For LOAD command

  while (Serial.available()) {
    char c = Serial.read();
    
    // Check for line termination
    if (c == '\n' || c == '\r') {
      if (pos == 0) continue;  // skip empty lines
      buf[pos] = '\0';
      pos = 0;

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
        }
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
        // Z<offset> (-12 to +14)
        int z = atoi(buf + 1);
        if (z >= -12 && z <= 14) {
          // Store as value+12 to keep it as uint8 (0-26)
          EEPROM.update(ADDR_TZ_OFFSET, z + 12);
          tzOffset = z + 12;
          Serial.print("OK:Z");
          Serial.println(z);
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
        }
      }
      else if (buf[0] == 'L' && buf[1] == 'O' && buf[2] == 'A' && buf[3] == 'D') {
        // LOAD<binary_data>
        // Format: L<length_byte><40_bytes_of_DST_data>
        // For simplicity, we expect exactly 40 bytes after "LOAD:"
        if (pos >= 45) {  // "LOAD" (4) + ":" (1) + 40 bytes = 45
          // Extract 40 bytes starting after "LOAD:"
          uint8_t i = 0;
          char* ptr = buf + 5;  // skip "LOAD:"
          for (i = 0; i < 40 && *ptr; i++) {
            binaryBuffer[i] = (uint8_t)(*ptr);
            ptr++;
          }
          if (i == 40) {
            parseDSTTable(binaryBuffer, 40);
          }
        }
        // Note: In practice, binary upload would use Web Serial's writableStreamDefaultWriter
        // and send raw bytes. This is a simplified ASCII fallback.
      }

      return;
    }
    else if (pos < sizeof(buf) - 1) {
      buf[pos++] = c;
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
  
  // Load timezone offset
  uint8_t tzByte = EEPROM.read(ADDR_TZ_OFFSET);
  if (tzByte == 0 || tzByte > 26) {
    tzOffset = 12;  // Default UTC+0
  } else {
    tzOffset = tzByte;
  }
  
  // Initialize date checking for auto-increment
  lastDateCheck = rtc.now();
  lastMillis = millis();
  
  // Check initial DST status
  checkAndApplyDST();

  updateDisplay();
}

void loop() {
  if (Serial.available()) handleSerial();
  
  // Auto-increment date every 24 hours
  autoIncrementDate();
  
  updateDisplay();
  delay(500);
}
