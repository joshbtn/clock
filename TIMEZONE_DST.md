# Timezone & DST Documentation

## Overview

The clock uses **algorithm-based DST** instead of static lookup tables. This means the Arduino firmware calculates DST transitions on-the-fly based on region rules, eliminating the need to upload 40-byte DST tables and support future rule changes indefinitely.

**Key principle:** The browser sends only the timezone ID (0-12) to the Arduino. The Arduino uses that ID to determine which DST algorithm to apply.

---

## Timezone IDs and Definitions

All timezones are defined in `main.cpp` as a struct array:

```cpp
const Timezone timezones[] = {
  {0,   0, "UTC",                 0},  // No DST
  {1,  -5, "USA Eastern",         1},  // EST/EDT - DST rule 1
  {2,  -6, "USA Central",         1},  // CST/CDT - DST rule 1
  {3,  -7, "USA Mountain",        1},  // MST/MDT - DST rule 1
  {4,  -8, "USA Pacific",         1},  // PST/PDT - DST rule 1
  {5,  -3, "Canada Atlantic",     1},  // AST/ADT - DST rule 1
  {6,  -5, "Canada Eastern",      1},  // EST/EDT - DST rule 1
  {7,  -6, "Canada Central",      1},  // CST/CDT - DST rule 1
  {8,  -7, "Canada Mountain",     0},  // MST year-round - No DST
  {9,  -8, "Canada Pacific",      1},  // PST/PDT - DST rule 1
  {10,  0, "UK London",           2},  // GMT/BST - DST rule 2
  {11, -7, "Arizona",             0},  // MST year-round - No DST
  {12,-10, "Hawaii",              0},  // HST year-round - No DST
};
```

| ID | Region | UTC Offset | DST Rule | Notes |
|----|--------|-----------|----------|-------|
| 0 | UTC | 0 | None | Reference, no DST |
| 1 | USA Eastern | -5 | USA/Canada | EST/EDT transitions |
| 2 | USA Central | -6 | USA/Canada | CST/CDT transitions |
| 3 | USA Mountain | -7 | USA/Canada | MST/MDT transitions |
| 4 | USA Pacific | -8 | USA/Canada | PST/PDT transitions |
| 5 | Canada Atlantic | -3 | USA/Canada | AST/ADT transitions |
| 6 | Canada Eastern | -5 | USA/Canada | EST/EDT transitions |
| 7 | Canada Central | -6 | USA/Canada | CST/CDT transitions |
| 8 | Canada Mountain | -7 | None | No DST ever |
| 9 | Canada Pacific | -8 | USA/Canada | PST/PDT transitions |
| 10 | UK London | 0 | UK/EU | GMT/BST transitions |
| 11 | Arizona | -7 | None | No DST (year-round MST) |
| 12 | Hawaii | -10 | None | No DST (year-round HST) |

---

## DST Rules

### Rule 0: No DST

**Applied to:** UTC, Canada Mountain, Arizona, Hawaii

**Logic:**
```
dstActive = false  (always)
```

These regions never have DST. The Arduino stores the standard time offset permanently.

---

### Rule 1: USA/Canada (North American DST)

**Applied to:** USA (Eastern, Central, Mountain, Pacific) + most of Canada

**Transition Dates:**
- **Spring Forward:** 2nd Sunday in March @ 2:00 AM local time → 3:00 AM
- **Fall Back:** 1st Sunday in November @ 2:00 AM local time → 1:00 AM

**Examples:**
- 2026: Mar 8 (spring), Nov 1 (fall)
- 2027: Mar 14 (spring), Nov 7 (fall)
- 2028: Mar 12 (spring), Nov 5 (fall)

**Firmware Logic** (`isDSTActive_USA_Canada`):

```cpp
bool isDSTActive_USA_Canada(uint16_t year, uint8_t month, uint8_t day) {
  // March: DST from 2nd Sunday onward
  if (month == 3) {
    uint8_t secondSunday = getNthSunday(year, 3, 2);
    return day >= secondSunday;
  }
  
  // April-October: DST active all month
  if (month > 3 && month < 11) {
    return true;
  }
  
  // November: DST until 1st Sunday (not including 1st Sunday)
  if (month == 11) {
    uint8_t firstSunday = getNthSunday(year, 11, 1);
    return day < firstSunday;
  }
  
  // December-February: standard time
  return false;
}
```

**Time Zones Using This Rule:**
- USA: Eastern, Central, Mountain, Pacific
- Canada: Atlantic, Eastern, Central, Pacific

---

### Rule 2: UK/EU DST

**Applied to:** UK, most of Europe

**Transition Dates:**
- **Spring Forward:** Last Sunday in March @ 1:00 AM GMT → 2:00 AM BST
- **Fall Back:** Last Sunday in October @ 2:00 AM BST → 1:00 AM GMT

**Examples:**
- 2026: Mar 29 (spring), Oct 25 (fall)
- 2027: Mar 28 (spring), Oct 31 (fall)
- 2028: Mar 26 (spring), Oct 29 (fall)

**Firmware Logic** (`isDSTActive_UK`):

```cpp
bool isDSTActive_UK(uint16_t year, uint8_t month, uint8_t day) {
  // March: DST from last Sunday onward
  if (month == 3) {
    uint8_t lastSunday = getNthSunday(year, 3, -1);
    return day >= lastSunday;
  }
  
  // April-September: DST active
  if (month > 3 && month < 10) {
    return true;
  }
  
  // October: DST until last Sunday (not including last Sunday)
  if (month == 10) {
    uint8_t lastSunday = getNthSunday(year, 10, -1);
    return day < lastSunday;
  }
  
  // November-February: standard time
  return false;
}
```

**Time Zones Using This Rule:**
- UK London
- (Can be extended to other EU countries)

---

## Helper Functions

### 1. `getDayOfWeek(year, month, day)`

**Purpose:** Calculate day of week (0=Sunday, 1=Monday, ..., 6=Saturday)

**Algorithm:** Zeller's Congruence

```cpp
uint8_t getDayOfWeek(uint16_t year, uint8_t month, uint8_t day) {
  if (month < 3) {
    month += 12;
    year--;
  }
  uint16_t q = day;
  uint16_t m = month;
  uint16_t k = year % 100;
  uint16_t j = year / 100;
  uint16_t h = (q + ((13 * (m + 1)) / 5) + k + (k / 4) + (j / 4) - (2 * j)) % 7;
  // Zeller returns 0=Sat, 1=Sun, 2=Mon, ...
  // Convert to: 0=Sun, 1=Mon, ..., 6=Sat
  return (h + 5) % 7;
}
```

**Examples:**
- `getDayOfWeek(2026, 3, 8)` → 0 (Sunday) ✓
- `getDayOfWeek(2026, 11, 1)` → 0 (Sunday) ✓

---

### 2. `getNthSunday(year, month, n)`

**Purpose:** Find the Nth Sunday in a month (n=1 for first, n=-1 for last)

**Algorithm:**
1. Find day of week for the 1st of the month
2. Calculate offset to first Sunday
3. Add weeks to reach Nth Sunday
4. For last Sunday (-1), work backwards from the last day of month

```cpp
uint8_t getNthSunday(uint16_t year, uint8_t month, int8_t n) {
  uint8_t daysInMonth[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  
  // Check for leap year
  if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
    daysInMonth[2] = 29;
  }
  
  if (n > 0) {
    // Find the Nth Sunday
    uint8_t dow = getDayOfWeek(year, month, 1);
    uint8_t firstSunday = 1 + ((7 - dow) % 7);
    return firstSunday + ((n - 1) * 7);
  } else if (n == -1) {
    // Find last Sunday
    uint8_t lastDay = daysInMonth[month];
    uint8_t dow = getDayOfWeek(year, month, lastDay);
    return lastDay - dow;
  }
  
  return 1;
}
```

**Examples:**
- `getNthSunday(2026, 3, 2)` → 8 (2nd Sunday in March 2026)
- `getNthSunday(2026, 11, 1)` → 1 (1st Sunday in November 2026)
- `getNthSunday(2026, 10, -1)` → 25 (Last Sunday in October 2026)

---

## Firmware Flow

### Startup (`setup()`)

```
1. Load timezone ID from EEPROM[ADDR_TZ_ID]
   If invalid, default to 0 (UTC)

2. Call checkAndApplyDST()
   - Reads current RTC date/time
   - Looks up timezone's DST rule from timezones[] array
   - Calls appropriate algorithm (USA/Canada, UK, or None)
   - Sets global dstActive flag
   
3. Update display with current time
```

### Serial Command: Time & Date Sync

```
Browser sends:  T<h>,<m>,<s>         (time)
                D<month>,<day>,<year> (date)

Firmware:
1. Parse commands
2. Adjust RTC to new date/time
3. Call checkAndApplyDST() to recalculate based on new date
4. Send OK responses
```

### Serial Command: Timezone Change

```
Browser sends:  Z<tz_id>   (0-12)

Firmware:
1. Validate tz_id (must be 0 to NUM_TIMEZONES-1)
2. Save to EEPROM[ADDR_TZ_ID]
3. Update global tzId variable
4. Call checkAndApplyDST() to recalculate for new timezone
5. Send OK response with timezone name
```

### Automatic Date Increment

Every 24 hours (~86,400,000 ms), the firmware:
```
1. Increment RTC date by 1 day
2. Handle month/year overflow
3. Call checkAndApplyDST()
   - Recalculate DST status for the new date
   - This handles DST transitions correctly
```

---

## Web UI Flow

### Timezone Selection Process

1. **User selects timezone** from dropdown (ID 0-12)
2. **Local state updates:**
   - Timezone display shows name (from `timezoneConfig`)
   - DST rule display shows rule type (usa/uk/none)
   - No command sent yet

3. **User clicks "Sync Settings to Device"**
4. **Browser sends commands:**
   ```javascript
   Z<tz_id>\n     // Set timezone
   ```
5. **Device ACKs:**
   ```
   OK:Z<tz_id>
   DBG:TZ <name>
   ```
6. **Device auto-recalculates DST** based on current date + new timezone

### Timezone Configuration Table (Client-Side)

```javascript
const timezoneConfig = {
    0:  { name: 'UTC', offset: 0, dstRule: 'none' },
    1:  { name: 'USA Eastern', offset: -5, dstRule: 'usa' },
    2:  { name: 'USA Central', offset: -6, dstRule: 'usa' },
    // ... etc
    11: { name: 'Arizona', offset: -7, dstRule: 'none' },
    12: { name: 'Hawaii', offset: -10, dstRule: 'none' }
};
```

The browser uses this for **display only**. The actual DST calculations happen on the Arduino.

---

## Adding a New Timezone

### Step 1: Update Firmware (`main.cpp`)

Add entry to `timezones[]` array:

```cpp
{13, -9, "Alaska", 1},  // AKST/AKDT, follows USA rule
```

Update `NUM_TIMEZONES` automatically (it's calculated from array size).

Update the Z command error message if needed.

### Step 2: Update Web UI (`index.html`)

Add option to the timezone selector:

```html
<option value="13">Alaska (AKST/AKDT)</option>
```

Add entry to `timezoneConfig`:

```javascript
13: { name: 'Alaska', offset: -9, dstRule: 'usa' }
```

### Step 3: Build & Test

```bash
pio run --target build
pio run --target upload
```

Test by selecting the new timezone and clicking "Sync Settings".

---

## Memory Impact

| Component | Size | Notes |
|-----------|------|-------|
| Timezone array | ~200 bytes | Constant data in Flash |
| DST helper functions | ~500 bytes | Code |
| EEPROM usage | 1 byte | Stores timezone ID (ADDR_TZ_ID = 0x02) |
| **Total** | ~700 bytes | Much smaller than 40-byte DST table + logic |

**Before (table-based):** 40 bytes EEPROM + 300 bytes code = 340 bytes  
**After (algorithm-based):** 1 byte EEPROM + 500 bytes code = 501 bytes  
**Difference:** +161 bytes, but **infinitely scalable** and **self-updating** for future rule changes.

---

## Examples: DST Transitions

### 2026 - USA Eastern (Rule 1)

- **Spring:** Mar 8 (day 67 of year)
  - 2nd Sunday in March = Mar 8
  - Date >= Mar 8 → `dstActive = true`
  
- **Fall:** Nov 1 (day 305 of year)
  - 1st Sunday in November = Nov 1
  - Date < Nov 1 → `dstActive = false` on Nov 1 at 2 AM

**Calendar view:**
```
MARCH 2026
Su Mo Tu We Th Fr Sa
 1  2  3  4  5  6  7
 8  9 10 11 12 13 14  ← 2nd Sunday = Mar 8 (DST starts)
15 16 17 18 19 20 21
22 23 24 25 26 27 28
29 30 31

NOVEMBER 2026
Su Mo Tu We Th Fr Sa
 1  2  3  4  5  6  7  ← 1st Sunday = Nov 1 (DST ends)
 8  9 10 11 12 13 14
15 16 17 18 19 20 21
22 23 24 25 26 27 28
29 30
```

### 2026 - UK London (Rule 2)

- **Spring:** Mar 29 (day 88 of year)
  - Last Sunday in March = Mar 29
  - Date >= Mar 29 → `dstActive = true`
  
- **Fall:** Oct 25 (day 298 of year)
  - Last Sunday in October = Oct 25
  - Date < Oct 25 → `dstActive = false` on Oct 25 at 2 AM

**Calendar view:**
```
MARCH 2026
Su Mo Tu We Th Fr Sa
 1  2  3  4  5  6  7
 8  9 10 11 12 13 14
15 16 17 18 19 20 21
22 23 24 25 26 27 28
29 30 31             ← Last Sunday = Mar 29 (DST starts)

OCTOBER 2026
Su Mo Tu We Th Fr Sa
 4  5  6  7  8  9 10
11 12 13 14 15 16 17
18 19 20 21 22 23 24
25 26 27 28 29 30 31 ← Last Sunday = Oct 25 (DST ends)
```

---

## Testing Checklist

- [ ] Firmware compiles without errors
- [ ] Device accepts `Z<id>` commands for all 13 timezones (0-12)
- [ ] Device responds with `OK:Z<id>` and `DBG:TZ <name>`
- [ ] DST algorithm correctly identifies spring transition (2nd Sunday March for USA)
- [ ] DST algorithm correctly identifies fall transition (1st Sunday November for USA)
- [ ] No-DST timezones (0, 8, 11, 12) never report `dstActive = true`
- [ ] DST status updates when date is changed via `D` command
- [ ] DST status persists correctly after auto-date-increment

---

## Future Extensions

To add more timezones:

1. **Australia:** Last Sunday September (spring) to Last Sunday March (fall) — creates new rule 3
2. **South Africa:** No DST (year-round SAST)
3. **Brazil:** Complex rules varying by region — would need multiple IDs
4. **Middle East (UAE, Saudi Arabia):** No DST — add single rule with local offset

The modular design makes adding new rules simple. Just add the algorithm function and reference it in the timezone array.

---

## References

- Zeller's Congruence: https://en.wikipedia.org/wiki/Zeller%27s_congruence
- US DST Rules (current): 2nd Sun Mar – 1st Sun Nov
- UK DST Rules: Last Sun Mar – Last Sun Oct
- EEPROM Layout: See [AGENTS.md](AGENTS.md) for hardware reference
