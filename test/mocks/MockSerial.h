#pragma once
#include <string>
#include <queue>
#include <cstring>

// Mock Serial implementation for testing command parsing
class MockSerialClass {
private:
    std::queue<char> inputBuffer;
    std::string outputBuffer;

public:
    MockSerialClass() {}

    // Simulate Serial.available()
    int available() {
        return inputBuffer.size();
    }

    // Simulate Serial.read()
    int read() {
        if (inputBuffer.empty()) return -1;
        char c = inputBuffer.front();
        inputBuffer.pop();
        return (int)c;
    }

    // Simulate Serial.print()
    void print(const char* s) {
        outputBuffer += s;
    }

    void print(int value) {
        outputBuffer += std::to_string(value);
    }

    void print(char c) {
        outputBuffer += c;
    }

    // Simulate Serial.println()
    void println(const char* s = "") {
        outputBuffer += s;
        outputBuffer += "\n";
    }

    void println(int value) {
        outputBuffer += std::to_string(value);
        outputBuffer += "\n";
    }

    // Test helpers
    void setInput(const char* input) {
        for (const char* p = input; *p; ++p) {
            inputBuffer.push(*p);
        }
    }

    std::string getOutput() const {
        return outputBuffer;
    }

    void clearOutput() {
        outputBuffer.clear();
    }

    void clearInput() {
        while (!inputBuffer.empty()) {
            inputBuffer.pop();
        }
    }
};

extern MockSerialClass MockSerial;
