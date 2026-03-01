#pragma once
#include <string.h>

// Mock EEPROM implementation using a simple array
class MockEEPROMClass {
private:
    static constexpr int EEPROM_SIZE = 1024;
    uint8_t data[EEPROM_SIZE];

public:
    MockEEPROMClass() {
        std::memset(data, 0, EEPROM_SIZE);
    }

    uint8_t read(int address) {
        if (address < 0 || address >= EEPROM_SIZE) return 0;
        return data[address];
    }

    void write(int address, uint8_t value) {
        if (address < 0 || address >= EEPROM_SIZE) return;
        data[address] = value;
    }

    void update(int address, uint8_t value) {
        // In real EEPROM, update only writes if value differs
        // For testing, just write
        write(address, value);
    }

    void clear() {
        std::memset(data, 0, EEPROM_SIZE);
    }
};

extern MockEEPROMClass MockEEPROM;
