# clock

A digital clock for **Arduino Nano** using a **TM1637** 4-digit 7-segment display and a **DS3231** real-time clock (RTC) module.

## Features

- Displays current time in **HH:MM** format
- Colon blinks every second
- Time is kept accurately by the DS3231 hardware RTC (battery-backed)

## Required Libraries

Install both libraries via the Arduino IDE **Library Manager** (`Sketch → Include Library → Manage Libraries…`):

| Library | Author |
|---------|--------|
| **TM1637Display** | Avishay Orpaz |
| **RTClib** | Adafruit |

## Wiring

### TM1637 Display → Arduino Nano

| TM1637 | Arduino Nano |
|--------|--------------|
| CLK    | D2           |
| DIO    | D3           |
| VCC    | 5V           |
| GND    | GND          |

### DS3231 RTC → Arduino Nano

| DS3231 | Arduino Nano |
|--------|--------------|
| SDA    | A4           |
| SCL    | A5           |
| VCC    | 3.3V (or 5V if module has onboard regulator) |
| GND    | GND          |

## Setting the Time

The first time you upload the sketch, uncomment the `rtc.adjust(…)` line in `setup()`:

```cpp
rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
```

Upload the sketch, then **immediately re-comment that line and upload again** to prevent the time from being reset on every power cycle.

## Uploading

1. Open `clock/clock.ino` in the Arduino IDE.
2. Select **Board → Arduino Nano** and the correct **Port**.
3. Click **Upload**.

## Troubleshooting

| Symptom | Likely cause |
|---------|--------------|
| Display shows `----` | DS3231 not found – check I2C wiring (SDA/SCL) |
| Display shows `00:00` | RTC not set – run `rtc.adjust(…)` once |
| Display blank | Check TM1637 wiring and power |