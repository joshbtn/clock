#pragma once

// Minimal mock DateTime class compatible with RTClib's DateTime
// Used for testing DST and date handling logic without hardware

struct MockDateTime {
    uint16_t _year;
    uint8_t _month;
    uint8_t _day;
    uint8_t _hour;
    uint8_t _minute;
    uint8_t _second;

    MockDateTime() : _year(2026), _month(1), _day(1), _hour(0), _minute(0), _second(0) {}

    MockDateTime(uint16_t year, uint8_t month, uint8_t day,
                 uint8_t hour = 0, uint8_t minute = 0, uint8_t second = 0)
        : _year(year), _month(month), _day(day),
          _hour(hour), _minute(minute), _second(second) {}

    uint16_t year() const { return _year; }
    uint8_t month() const { return _month; }
    uint8_t day() const { return _day; }
    uint8_t hour() const { return _hour; }
    uint8_t minute() const { return _minute; }
    uint8_t second() const { return _second; }

    // For convenience in tests
    bool operator==(const MockDateTime& other) const {
        return _year == other._year && _month == other._month && _day == other._day &&
               _hour == other._hour && _minute == other._minute && _second == other._second;
    }
};
