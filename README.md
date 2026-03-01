# Arduino Web-Serial Clock

A minimalist, internet-connected digital clock with automatic timezone and daylight savings handling. No buttons, no configuration—just open a browser and sync.

**Web Serial Interface:** [https://joshbtn.github.io/clock/](https://joshbtn.github.io/clock/)

## Design Principles

This project demonstrates a philosophy: **the UI does the thinking, the Arduino just displays what it's told.**

- **Buttonless**: All configuration (time, brightness, 24/12-hour format, AM/PM display) is managed via Web Serial from a Chrome browser. The Arduino has no physical controls.
- **Automatic Timezone & DST**: The browser calculates local time and automatically accounts for daylight savings. The Arduino stores local time directly, eliminating timezone math from firmware.
- **Flexible Time Display**: Support for both 24-hour and 12-hour (with AM/PM) modes via the web dashboard.
- **Persistent State**: Brightness setting survives power loss thanks to EEPROM storage.
- **Minimal Firmware**: Simple loop with no interrupts or sleep—~80 lines of straightforward code.

## Hardware

Built around three I2C/serial components:

| Component | Purpose |
|-----------|---------|
| **Arduino Nano** (ATmega328P) | Microcontroller—reads RTC, drives display, listens for serial commands |
| **TM1637 4-Digit Display** | 4-digit, 7-segment LED output (assumes colon `:` on digits 1 & 2 for `HH:MM` format) |
| **DS3231 RTC** | Accurate real-time clock with battery backup; keeps time during power loss |

The firmware is hardware-aware: every design decision (time format, EEPROM layout, I2C addresses, serial baudrate) is specific to this stack.

## Serial Protocol

Commands sent at 9600 baud, terminated with `\n`. All responses echo status (e.g., `OK:T`, `ERR:B expected 0..7`):

| Command | Example | Description |
|---------|---------|-------------|
| `T<h>,<m>,<s>` | `T19,58,32` | Set RTC to local time (hours, minutes, seconds, 24-hour) |
| `D<m>,<d>,<y>` | `D3,1,2026` | Set RTC date (month, day, year) |
| `F<0\|1>` | `F1` | Set time format (0=24-hour, 1=12-hour with AM/PM) |
| `QF` | `QF` | Query stored format setting |
| `Z<id>` | `Z1` | Set timezone by ID (0-20); triggers DST calculation. See [TIMEZONE_DST.md](TIMEZONE_DST.md) |
| `B<0-7>` | `B5` | Set display brightness (0=dimmest, 7=brightest) |

## File Layout

```
src/main.cpp      — Arduino firmware
www/index.html    — Web Serial dashboard
AGENTS.md         — Full architecture notes
```