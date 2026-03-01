#include <Wire.h>
#include <EEPROM.h>
#include <RTClib.h>
#include <TM1637Display.h>
#include <avr/sleep.h>

// Pins
#define CLK 3
#define DIO 4
#define WAKE_PIN 2

TM1637Display display(CLK, DIO);
RTC_DS3231 rtc;

// EEPROM Addresses
const int ADDR_TZ_OFFSET_LOW = 0;  // Low byte of timezone offset in minutes
const int ADDR_TZ_OFFSET_HIGH = 1; // High byte of timezone offset in minutes
const int ADDR_BRIGHTNESS = 2;     // 0-7 brightness level

// Helper function to read signed 16-bit timezone offset from EEPROM
int16_t readTimezoneOffset() {
  int16_t offset = (EEPROM.read(ADDR_TZ_OFFSET_HIGH) << 8) | EEPROM.read(ADDR_TZ_OFFSET_LOW);
  // Default to -300 (EST, -5 hours) if uninitialized
  if (offset == -1 || offset < -720 || offset > 720) {
    offset = -300;
  }
  return offset;
}

// Helper function to write signed 16-bit timezone offset to EEPROM
void writeTimezoneOffset(int16_t offset) {
  EEPROM.update(ADDR_TZ_OFFSET_LOW, offset & 0xFF);
  EEPROM.update(ADDR_TZ_OFFSET_HIGH, (offset >> 8) & 0xFF);
}

volatile bool wakeUp = true;

// Forward declarations
void handleSerial();

void setup() {
  Serial.begin(9600);
  Wire.begin();
  rtc.begin();
  
  pinMode(WAKE_PIN, INPUT_PULLUP);
  
  // Load brightness from EEPROM (default 5 if unset)
  uint8_t brightness = EEPROM.read(ADDR_BRIGHTNESS);
  if (brightness > 7) brightness = 5; // Default to mid-level
  display.setBrightness(brightness);
  
  // Enable SQW at 1Hz to trigger wake interrupt every second
  // This allows both periodic display updates AND serial responsiveness
  rtc.writeSqwPinMode(DS3231_SquareWave1Hz);
  rtc.disable32K();
}

void wakeHandler() {
  wakeUp = true;
}

void goToSleep() {
  // Use IDLE mode instead of PWR_DOWN to keep USART active for serial commands
  // Display stays on (USB powered, no need to turn off)
  set_sleep_mode(SLEEP_MODE_IDLE);
  sleep_enable();
  attachInterrupt(digitalPinToInterrupt(WAKE_PIN), wakeHandler, FALLING);
  sleep_cpu();
  
  // Wakes up here
  sleep_disable();
  detachInterrupt(digitalPinToInterrupt(WAKE_PIN));
}

void loop() {
  if (Serial.available() > 0) {
    handleSerial();
  }

  if (wakeUp) {
    DateTime now = rtc.now();
    time_t utc = now.unixtime();
    
    // Apply timezone offset from EEPROM (in minutes)
    int16_t tzOffset = readTimezoneOffset();
    time_t local = utc + (tzOffset * 60);
    
    // Extract hours and minutes from local time
    int hrs = (local / 3600) % 24;
    int mins = (local / 60) % 60;
    int displayTime = (hrs * 100) + mins;
    
    // Load brightness from EEPROM
    uint8_t brightness = EEPROM.read(ADDR_BRIGHTNESS);
    if (brightness > 7) brightness = 5;
    display.setBrightness(brightness);
    
    display.showNumberDecEx(displayTime, 0b01000000, true);
    
    wakeUp = false;
    delay(100); // Brief delay then back to sleep (wakes every 1s via RTC)
    goToSleep();
  }
}

void handleSerial() {
  char cmd = Serial.read();
  
  if (cmd == 'T') { // Sync Time (Unix)
    uint32_t t = Serial.parseInt();
    if (t > 0) {
      rtc.adjust(DateTime(t));
      wakeUp = true;
      Serial.print("OK:T");
      Serial.println(t);
    }
  }
  else if (cmd == 'Z') { // Set Timezone Offset (in minutes)
    int16_t offset = Serial.parseInt();
    if (offset >= -720 && offset <= 720) { // Validate range (-12h to +12h)
      writeTimezoneOffset(offset);
      wakeUp = true; // Refresh display with new timezone
      Serial.print("OK:Z");
      Serial.println(offset);
    }
  }
  else if (cmd == 'B') { // Set Brightness (0-7)
    uint8_t brightness = Serial.parseInt();
    if (brightness <= 7) {
      EEPROM.update(ADDR_BRIGHTNESS, brightness);
      display.setBrightness(brightness);
      wakeUp = true; // Refresh display to show new brightness
      Serial.print("OK:B");
      Serial.println(brightness);
    }
  }
  
  // Clear any remaining serial buffer
  while (Serial.available() > 0) {
    Serial.read();
  }
}
