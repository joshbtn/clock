# Project Logic: "Agents" and Automated Routines

This project utilizes several automated "agents" or logic blocks to ensure the clock remains accurate and energy-efficient without user intervention.

## 1. The Sleep/Wake Cycle Agent

To maximize the lifespan of the components and minimize power draw, the system follows a strict interrupt-driven cycle.

**Trigger:** The SQW pin on the DS3231 RTC is programmed to pulse at the start of every minute ($1/60 \text{Hz}$).

**Action:**
- The pulse triggers a Hardware Interrupt on Arduino Pin D2.
- The Nano wakes from `SLEEP_MODE_PWR_DOWN`.
- The Nano reads the current UTC time from the RTC via I2C (A4/A5).
- The display is updated via the TM1637 driver.
- The Nano immediately re-enters Deep Sleep.

## 2. The Timezone & DST Agent

Because the RTC only tracks raw time, this logic block performs the "Local Time" transformation.

**Storage:** The Timezone Offset and DST Rules are stored in the EEPROM (Addresses 0x00 and 0x01).

**Calculation:**
- The Timezone library calculates the transition dates (e.g., "Second Sunday in March") mathematically.
- If `CurrentDate > DST_Start` and `CurrentDate < DST_End`, the agent applies a $+3600\text{s}$ offset.

**Result:** The clock automatically "springs forward" and "falls back" without being plugged into a computer.

## 3. The Web Serial Sync Agent

This is the bridge between the physical hardware and the digital dashboard.

**Protocol:** Serial at 9600 Baud via the Nano's USB port.

**Command Set:**
- `T[UnixTimestamp]`: Adjusts the RTC internal clock.
- `Z[Offset]`: Updates the local timezone stored in EEPROM.

**Automation:** The `dashboard.html` interface uses the Browser's `Intl.DateTimeFormat().resolvedOptions().timeZone` to detect the user's location and automatically format the `Z` command before sending.

## 4. Optical Filtering Agent

**Component:** Dark Smoke Acrylic.

**Logic:** The acrylic acts as a high-pass filter. It blocks the low-intensity reflections of the internal PCBs and wires but allows the high-intensity yellow light from the TM1637 segments to pass through, creating a "Void Display" effect.
