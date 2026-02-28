# Ultra-Compact Bedroom Clock (Yellow Edition)

An ultra-minimalist, high-precision digital clock designed for the bedroom. This project focuses on a tiny footprint, eye-friendly yellow LED illumination, and zero-maintenance timekeeping through automatic Daylight Savings (DST) adjustments.

## Key Features

- **Warm Glow**: Uses a Yellow 0.56" TM1637 Display for minimal blue light exposure.
- **Set-and-Forget**: Automatically handles DST and timezones via software logic stored in the Nano's 1KB EEPROM.
- **Deep Sleep Power Saving**: The Arduino Nano stays in a low-power state, waking only once per minute via an interrupt from the DS3231 RTC.
- **Web Serial Sync**: No physical buttons. Sync the time, timezone, and brightness settings directly from a mobile Chrome browser using the included Dark Mode Dashboard.
- **Stealth Design**: Fits behind Dark Smoke Acrylic for a "hidden-until-lit" appearance.

## Hardware Stack

| Component | Role | Link |
|-----------|------|------|
| Arduino Nano | System Brain & Power Management | [View Product](#) |
| TM1637 Display | 4-Digit Yellow LED Output | [View Product](#) |
| DS3231 RTC | High-Precision I2C Timekeeper | [View Product](#) |
| Smoke Acrylic | Optical Filter & Front Panel | [View Product](#) |

## Project Structure

- **clock_assembly_guide.md**: Step-by-step wiring and assembly instructions.
- **dashboard.html**: Single-file Web Serial interface with Dark Mode.
- **Agents.md**: Documentation of the internal logic and automated routines.

## Dimensions

- **Target Footprint**: 55mm x 30mm x 25mm.
- **Enclosure**: Custom 3D-printed "Sandwich" stack.