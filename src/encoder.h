#ifndef ENCODER_H
#define ENCODER_H

#include <Arduino.h>
#include "config.h"

// Single wheel encoder for odometry
// FIX BUG 3: Tracks motor direction to count forward/backward correctly.

#define ENCODER_PIN        3        // GPIO3 (RX)
#define TICKS_PER_REV      20
#define WHEEL_DIAMETER_CM  6.5
#define WHEEL_CIRCUMF_CM   20.42    // PI * 6.5
#define CM_PER_TICK        1.021    // WHEEL_CIRCUMF / TICKS_PER_REV

// Volatile counter for ISR - now tracks signed direction
volatile long encoderCount = 0;
volatile int8_t encoderDir = 1;  // +1 = forward, -1 = backward, 0 = stopped

void IRAM_ATTR encoderISR() {
    encoderCount += encoderDir;
}

class Encoder {
public:
    float posX = 0;
    float posY = 0;
    float totalDistCm = 0;

    void begin() {
        Serial.end();

        pinMode(ENCODER_PIN, INPUT_PULLUP);
        attachInterrupt(digitalPinToInterrupt(ENCODER_PIN),
                        encoderISR, RISING);

        encoderCount = 0;
        encoderDir = 1;
        prevCount = 0;
    }

    // Call from motors to set current direction
    // Must be called whenever motor direction changes
    static void setDirection(int8_t dir) {
        noInterrupts();
        encoderDir = dir; // +1=forward, -1=backward, 0=stopped/turning
        interrupts();
    }

    void update(float headingRad) {
        noInterrupts();
        long count = encoderCount;
        interrupts();

        long delta = count - prevCount;
        prevCount = count;

        float distCm = delta * CM_PER_TICK; // now signed!
        totalDistCm += abs(distCm);

        // Update position using heading
        posX += distCm * sin(headingRad);
        posY += distCm * cos(headingRad);
    }

    // FIX BUG 7: Use floor() instead of int cast for negative coords
    int getGridX() {
        return (int)floor(posX / CELL_SIZE_CM) + (GRID_SIZE / 2);
    }

    int getGridY() {
        return (GRID_SIZE / 2) - (int)floor(posY / CELL_SIZE_CM);
    }

    void resetPosition() {
        noInterrupts();
        encoderCount = 0;
        interrupts();
        prevCount = 0;
        posX = 0;
        posY = 0;
        totalDistCm = 0;
    }

    long getCount() { return encoderCount; }

private:
    long prevCount = 0;
};

#endif // ENCODER_H
