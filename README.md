# Ultra-Compact Bedroom Clock (Yellow Edition)

An ultra-minimalist, high-precision digital clock designed for the bedroom. Tiny footprint, eye-friendly yellow LED illumination, and simple Web Serial time sync from any Chrome browser.

**Web Serial Interface:** [https://joshbtn.github.io/clock/](https://joshbtn.github.io/clock/)

## Key Features

- **Warm Glow**: Yellow 0.56" TM1637 display for minimal blue light exposure.
- **Web Serial Sync**: No physical buttons. Set time and brightness from a mobile Chrome browser using the included Dark Mode dashboard.
- **Battery-Backed RTC**: DS3231 with coin cell keeps accurate time through power outages.
- **Persistent Brightness**: Display brightness (0–7) saved to EEPROM, survives reboots.
- **Stealth Design**: Fits behind Dark Smoke Acrylic for a "hidden-until-lit" appearance.

## Hardware Stack

| Component | Role |
|-----------|------|
| Arduino Nano | Microcontroller (ATmega328P) |
| TM1637 Display | 4-Digit Yellow LED Output |
| DS3231 RTC | Battery-Backed I2C Timekeeper |
| Smoke Acrylic | Optical Filter & Front Panel |

## Serial Command Protocol

All commands are sent at 9600 baud, terminated with `\n`.

| Command | Example | Description |
|---------|---------|-------------|
| `T<h>,<m>,<s>` | `T19,58,32` | Set local time (hours, minutes, seconds) |
| `B<0-7>` | `B5` | Set display brightness |
| `Z<offset>` | `Z-300` | Acknowledged only (timezone handled by UI) |

## Project Structure

```
src/main.cpp    — Arduino firmware (~75 lines)
www/index.html  — Web Serial dashboard (GitHub Pages)
AGENTS.md       — Architecture documentation
```

## Dimensions

- **Target Footprint**: 55mm x 30mm x 25mm
- **Enclosure**: Custom 3D-printed "Sandwich" stack