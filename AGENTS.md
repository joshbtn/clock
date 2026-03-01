# Project Architecture

Simple, maintainable design. The UI does the thinking, the Arduino just displays what it's told.

## 1. Main Loop

The Arduino runs a straightforward loop with no sleep or interrupts:

1. Check for incoming serial commands.
2. Read time from DS3231 RTC via I2C.
3. Display `HH:MM` on the TM1637.
4. Wait 500ms, repeat.

## 2. Time & Timezone Sync

The Arduino firmware implements **algorithm-based DST** and timezone support via timezone IDs; the browser calculates current local time and sends it. See [TIMEZONE_DST.md](TIMEZONE_DST.md) for full details on supported timezones and DST rule logic.

**Flow:**
1. User opens the Web Serial dashboard in Chrome.
2. Browser reads local time via `new Date()`.
3. Browser sends `T<hour>,<minute>,<second>\n` and `D<month>,<day>,<year>\n` over serial.
4. Browser sends `Z<timezone_id>\n` to select timezone and DST rules.
5. Arduino sets the DS3231 RTC and calculates DST status on-the-fly.
6. DS3231's coin cell battery keeps time through power outages.

**Key decision:** Algorithm-based DST means the Arduino firmware determines DST status from current date and timezone rules, eliminating the need to upload 40+ byte DST lookup tables.

## 3. Brightness Control

Brightness (0–7) is stored in EEPROM address `0x00` and persists across reboots.

**Flow:**
1. Browser sends `B<0-7>\n` over serial.
2. Arduino writes to EEPROM and updates the TM1637.
3. On boot, brightness is loaded from EEPROM (default: 5).

## 4. Serial Protocol

Line-buffered at 9600 baud. Commands are a single letter followed by optional comma-separated data, terminated with `\n`. For example: `T19,58,32\n`, `B5\n`, `Z1\n`.

The Arduino reads characters into a buffer until `\n`, then parses the complete line with `sscanf` / `atoi`. This avoids issues with `Serial.parseInt()` timeouts and partial reads. All commands echo a status response: `OK:<cmd>` on success, or `ERR:<msg>` on failure.

## 5. Display Format

Supports both 24-hour and 12-hour (with AM/PM) time display, configurable via the `F` command and persisted in EEPROM. The TM1637 shows `HH:MM` with the colon lit between digits 1 and 2.
