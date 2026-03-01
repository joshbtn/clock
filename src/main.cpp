#include <Wire.h>
#include <EEPROM.h>
#include <RTClib.h>
#include <TM1637Display.h>

// Pins
#define CLK 3
#define DIO 4

TM1637Display display(CLK, DIO);
RTC_DS3231 rtc;

// EEPROM address for brightness (0-7)
const int ADDR_BRIGHTNESS = 0;

void handleSerial();
void updateDisplay();

void setup() {
  Serial.begin(9600);
  Wire.begin();
  rtc.begin();

  // If DS3231 battery died, default to midnight
  if (rtc.lostPower()) {
    rtc.adjust(DateTime(2026, 1, 1, 0, 0, 0));
  }

  // Load brightness from EEPROM (default 5 if unset)
  uint8_t brightness = EEPROM.read(ADDR_BRIGHTNESS);
  if (brightness > 7) brightness = 5;
  display.setBrightness(brightness);

  // Show time immediately on boot
  updateDisplay();
}

void loop() {
  // Check for serial commands
  if (Serial.available() > 0) {
    handleSerial();
  }

  // Update display every loop (TM1637 handles redundant writes fine)
  updateDisplay();

  delay(500); // Update twice per second
}

void updateDisplay() {
  DateTime now = rtc.now();
  int displayTime = (now.hour() * 100) + now.minute();
  display.showNumberDecEx(displayTime, 0b01000000, true);
}

void handleSerial() {
  char cmd = Serial.read();

  if (cmd == 'T') {
    // UI sends local time as Unix timestamp
    uint32_t t = Serial.parseInt();
    if (t > 0) {
      rtc.adjust(DateTime(t));
      updateDisplay();
      Serial.print("OK:T");
      Serial.println(t);
    }
  }
  else if (cmd == 'Z') {
    // Just acknowledge (UI already adjusted for timezone)
    int offset = Serial.parseInt();
    Serial.print("OK:Z");
    Serial.println(offset);
  }
  else if (cmd == 'B') {
    uint8_t brightness = Serial.parseInt();
    if (brightness <= 7) {
      EEPROM.update(ADDR_BRIGHTNESS, brightness);
      display.setBrightness(brightness);
      updateDisplay();
      Serial.print("OK:B");
      Serial.println(brightness);
    }
  }

  // Flush remaining serial buffer
  while (Serial.available() > 0) {
    Serial.read();
  }
}
