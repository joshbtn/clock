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
  char cmd = Serial.read();

  if (cmd == 'T') {
    // T<hour>,<minute>,<second>  — set local time
    int h = Serial.parseInt();
    int m = Serial.parseInt();
    int s = Serial.parseInt();
    Serial.print("DBG:T h="); Serial.print(h);
    Serial.print(" m="); Serial.print(m);
    Serial.print(" s="); Serial.println(s);
    if (h >= 0 && h <= 23 && m >= 0 && m <= 59 && s >= 0 && s <= 59) {
      DateTime now = rtc.now();
      rtc.adjust(DateTime(now.year(), now.month(), now.day(), h, m, s));
      updateDisplay();
      Serial.print("OK:T");
      Serial.print(h); Serial.print(":");
      Serial.print(m); Serial.print(":");
      Serial.println(s);
    }
  }
  else if (cmd == 'B') {
    // B<0-7>  — set brightness
    uint8_t b = Serial.parseInt();
    if (b <= 7) {
      EEPROM.update(ADDR_BRIGHTNESS, b);
      display.setBrightness(b);
      updateDisplay();
      Serial.print("OK:B");
      Serial.println(b);
    }
  }
  else if (cmd == 'Z') {
    // Acknowledge only (timezone handled by UI)
    Serial.print("OK:Z");
    Serial.println(Serial.parseInt());
  }

  while (Serial.available()) Serial.read();
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
