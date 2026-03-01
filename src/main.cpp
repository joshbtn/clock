#include <Wire.h>
#include <EEPROM.h>
#include <RTClib.h>
#include <TM1637Display.h>
#include <Timezone.h>
#include <avr/sleep.h>

// Pins
#define CLK 3
#define DIO 4
#define WAKE_PIN 2

TM1637Display display(CLK, DIO);
RTC_DS3231 rtc;

// EEPROM Addresses
const int ADDR_TZ_OFFSET = 0;
const int ADDR_DST_MODE = 1; // 0: Manual, 1: Auto

// Timezone Rules (Default: US Eastern)
TimeChangeRule usEDT = {"EDT", Second, Sun, Mar, 2, -240};
TimeChangeRule usEST = {"EST", First, Sun, Nov, 2, -300};
Timezone myTZ(usEDT, usEST);

volatile bool wakeUp = true;

// Forward declarations
void handleSerial();

void setup() {
  Serial.begin(9600);
  Wire.begin();
  rtc.begin();
  
  pinMode(WAKE_PIN, INPUT_PULLUP);
  display.setBrightness(0x0a);
  
  // Set SQW to 1Hz or 1-minute pulse if supported, 
  // but for simplicity, we wake on signal.
  rtc.writeSqwPinMode(DS3231_OFF); // Reset
  rtc.disable32K();
}

void wakeHandler() {
  wakeUp = true;
}

void goToSleep() {
  display.setBrightness(0x00, false); // Turn off display for extra saving
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
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
    time_t local = myTZ.toLocal(utc);
    
    int displayTime = (hour(local) * 100) + minute(local);
    display.setBrightness(0x0a);
    display.showNumberDecEx(displayTime, 0b01000000, true);
    
    wakeUp = false;
    delay(2000); // Wait for visibility
    goToSleep();
  }
}

void handleSerial() {
  char cmd = Serial.read();
  if (cmd == 'T') { // Sync Time (Unix)
    uint32_t t = Serial.parseInt();
    rtc.adjust(DateTime(t));
    wakeUp = true;
  }
  if (cmd == 'Z') { // Set Timezone Offset
    int offset = Serial.parseInt();
    EEPROM.update(ADDR_TZ_OFFSET, offset);
  }
}
