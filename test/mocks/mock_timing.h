#pragma once

// Mock timing functions for testable time-dependent code
class MockTiming {
private:
    static unsigned long mockMillis;

public:
    // Get the mocked value of millis()
    static unsigned long millis() {
        return mockMillis;
    }

    // Set mock time for testing
    static void setMillis(unsigned long ms) {
        mockMillis = ms;
    }

    // Advance mock time
    static void advanceMillis(unsigned long ms) {
        mockMillis += ms;
    }

    // Reset to 0
    static void reset() {
        mockMillis = 0;
    }
};

unsigned long MockTiming::mockMillis = 0;

// In tests, replace ::millis() with MockTiming::millis()
