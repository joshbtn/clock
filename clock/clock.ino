/*
 * clock.ino
 *
 * Digital clock using a TM1637 4-digit 7-segment display and DS3231 RTC
 * running on an Arduino Nano.
 *
 * Required libraries (install via Arduino Library Manager):
 *   - TM1637Display by Avishay Orpaz
 *   - RTClib by Adafruit
 *
 * Wiring:
 *   TM1637       Arduino Nano
 *   -------      ------------
 *   CLK    ----> D2
 *   DIO    ----> D3
 *   VCC    ----> 5V
 *   GND    ----> GND
 *
 *   DS3231       Arduino Nano
 *   -------      ------------
 *   SDA    ----> A4
 *   SCL    ----> A5
 *   VCC    ----> 3.3V (or 5V if module has onboard regulator)
 *   GND    ----> GND
 */

#include <TM1637Display.h>
#include <RTClib.h>

// TM1637 display pins
#define CLK_PIN 2
#define DIO_PIN 3

// Display brightness: 0 (lowest) to 7 (highest)
#define BRIGHTNESS 4

TM1637Display display(CLK_PIN, DIO_PIN);
RTC_DS3231 rtc;

void setup() {
  Serial.begin(9600);

  display.setBrightness(BRIGHTNESS);
  display.clear();

  if (!rtc.begin()) {
    Serial.println("DS3231 RTC not found. Check wiring.");
    // Show "----" on display to indicate error
    uint8_t dashPattern[] = {SEG_G, SEG_G, SEG_G, SEG_G};
    display.setSegments(dashPattern);
    while (true) {
      delay(1000);
    }
  }

  // Uncomment the line below (once) to set the RTC to the compile time,
  // then re-comment it to avoid resetting on every power cycle:
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
}

void loop() {
  DateTime now = rtc.now();

  int hours   = now.hour();
  int minutes = now.minute();
  int seconds = now.second();

  // Build a 4-digit value: HHMM
  int timeValue = hours * 100 + minutes;

  // Blink the colon every second
  bool showColon = (seconds % 2 == 0);

  display.showNumberDecEx(timeValue, showColon ? 0x40 : 0x00, true);

  delay(500);
}
