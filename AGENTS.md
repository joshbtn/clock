# Project Architecture

Simple, maintainable design. The UI does the thinking, the Arduino just displays what it's told.

## 1. Main Loop

The Arduino runs a straightforward loop with no sleep or interrupts:

1. Check for incoming serial commands.
2. Read time from DS3231 RTC via I2C.
3. Display `HH:MM` on the TM1637.
4. Wait 500ms, repeat.

## 2. Time Sync

The browser handles all timezone and DST logic. The Arduino stores **local time** directly.

**Flow:**
1. User opens the Web Serial dashboard in Chrome.
2. Browser reads local time via `new Date()`.
3. Browser sends `T<hour>,<minute>,<second>\n` over serial.
4. Arduino parses the line and sets the DS3231 RTC.
5. DS3231's coin cell battery keeps time through power outages.

**Key decision:** No timezone math on the Arduino. The browser already knows the user's local time — sending it directly eliminates an entire class of bugs.

## 3. Brightness Control

Brightness (0–7) is stored in EEPROM address `0x00` and persists across reboots.

**Flow:**
1. Browser sends `B<0-7>\n` over serial.
2. Arduino writes to EEPROM and updates the TM1637.
3. On boot, brightness is loaded from EEPROM (default: 5).

## 4. Serial Protocol

Line-buffered at 9600 baud. Commands are a single letter followed by data, terminated with `\n`.

The Arduino reads characters into a buffer until `\n`, then parses the complete line with `sscanf` / `atoi`. This avoids issues with `Serial.parseInt()` timeouts and partial reads.

## 5. Optical Filtering

**Component:** Dark Smoke Acrylic.

The acrylic blocks low-intensity reflections of internal PCBs and wires but passes the high-intensity yellow LED segments, creating a "floating digits" effect.
