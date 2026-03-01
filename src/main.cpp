#include <Wire.h>
#include <EEPROM.h>
#include <RTClib.h>
#include <TM1637Display.h>

#define CLK_PIN 3
#define DIO_PIN 4
#define ADDR_BRIGHTNESS 0

TM1637Display display(CLK_PIN, DIO_PIN);
RTC_DS3231 rtc;

void updateDisplay() {
  DateTime now = rtc.now();
  int time = now.hour() * 100 + now.minute();
  display.showNumberDecEx(time, 0b01000000, true);
}

void handleSerial() {
  // Read full line into buffer
  static char buf[32];
  static uint8_t pos = 0;

  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (pos == 0) continue; // skip empty
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
      else if (buf[0] == 'B') {
        int b = atoi(buf + 1);
        if (b >= 0 && b <= 7) {
          EEPROM.update(ADDR_BRIGHTNESS, b);
          display.setBrightness(b);
          updateDisplay();
          Serial.print("OK:B");
          Serial.println(b);
        }
      }
      else if (buf[0] == 'Z') {
        Serial.print("OK:Z");
        Serial.println(atoi(buf + 1));
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

  uint8_t brightness = EEPROM.read(ADDR_BRIGHTNESS);
  if (brightness > 7) brightness = 5;
  display.setBrightness(brightness);

  updateDisplay();
}

void loop() {
  if (Serial.available()) handleSerial();
  updateDisplay();
  delay(500);
}
