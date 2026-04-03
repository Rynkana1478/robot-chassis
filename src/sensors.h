#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>
#include <Wire.h>
#include <Servo.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include "config.h"

// HC-SR04 on servo sweep (non-blocking)
// MPU6050 gyro Z integration for heading (no compass)

enum SweepState {
    SWEEP_CENTER,
    SWEEP_MOVE_LEFT,
    SWEEP_READ_LEFT,
    SWEEP_MOVE_CENTER,
    SWEEP_READ_CENTER,
    SWEEP_MOVE_RIGHT,
    SWEEP_READ_RIGHT,
    SWEEP_RETURN,
    SWEEP_DONE
};

class Sensors {
public:
    float distFront = 999;
    float distLeft  = 999;
    float distRight = 999;
    float heading   = 0;      // degrees 0-360 (from gyro integration)
    float headingRad = 0;

    Servo sweepServo;
    Adafruit_MPU6050 mpu;

    SweepState sweepState = SWEEP_CENTER;
    unsigned long sweepTimer = 0;
    bool sweepActive = false;
    bool sweepComplete = false;

    // Gyro state
    float gyroZOffset = 0;
    unsigned long lastGyroUpdate = 0;
    bool gyroCalibrated = false;

    void begin() {
        Wire.begin(I2C_SDA, I2C_SCL);

        pinMode(US_TRIG, OUTPUT);
        pinMode(US_ECHO, INPUT);

        sweepServo.attach(SERVO_PIN);
        sweepServo.write(SERVO_CENTER);
        delay(300);

        initMPU();
    }

    // --- Front distance (only when servo at center) ---
    bool readFrontNow() {
        if (servoAtCenter()) {
            distFront = readUltrasonic();
            return true;
        }
        return false;
    }

    bool servoAtCenter() {
        return (sweepState == SWEEP_CENTER ||
                sweepState == SWEEP_READ_CENTER ||
                sweepState == SWEEP_DONE);
    }

    // --- Non-blocking sweep ---
    bool sweepTick() {
        unsigned long now = millis();
        switch (sweepState) {
            case SWEEP_CENTER:     return false;
            case SWEEP_MOVE_LEFT:  sweepServo.write(SERVO_LEFT);  sweepTimer = now; sweepState = SWEEP_READ_LEFT;  return false;
            case SWEEP_READ_LEFT:  if (now - sweepTimer >= SERVO_SWEEP_MS) { distLeft = readUltrasonic(); sweepState = SWEEP_MOVE_CENTER; } return false;
            case SWEEP_MOVE_CENTER:sweepServo.write(SERVO_CENTER);sweepTimer = now; sweepState = SWEEP_READ_CENTER;return false;
            case SWEEP_READ_CENTER:if (now - sweepTimer >= SERVO_SWEEP_MS) { distFront = readUltrasonic(); sweepState = SWEEP_MOVE_RIGHT; } return false;
            case SWEEP_MOVE_RIGHT: sweepServo.write(SERVO_RIGHT); sweepTimer = now; sweepState = SWEEP_READ_RIGHT; return false;
            case SWEEP_READ_RIGHT: if (now - sweepTimer >= SERVO_SWEEP_MS) { distRight = readUltrasonic(); sweepState = SWEEP_RETURN; }    return false;
            case SWEEP_RETURN:     sweepServo.write(SERVO_CENTER);sweepTimer = now; sweepState = SWEEP_DONE;       return false;
            case SWEEP_DONE:
                if (now - sweepTimer >= SERVO_SWEEP_MS) {
                    sweepState = SWEEP_CENTER;
                    sweepActive = false;
                    sweepComplete = true;
                    return true;
                }
                return false;
        }
        return false;
    }

    void startSweep() { if (!sweepActive) { sweepActive = true; sweepComplete = false; sweepState = SWEEP_MOVE_LEFT; } }
    bool isSweeping()  { return sweepActive; }
    bool isSweepDone() { return sweepComplete; }
    void clearSweepDone() { sweepComplete = false; }

    // --- Heading from gyro Z integration ---
    void updateHeading() {
        unsigned long now = millis();
        if (lastGyroUpdate == 0) { lastGyroUpdate = now; return; }

        float dt = (now - lastGyroUpdate) / 1000.0;
        lastGyroUpdate = now;

        sensors_event_t a, g, t;
        mpu.getEvent(&a, &g, &t);

        // rad/s to deg/s, remove bias
        float gz = (g.gyro.z - gyroZOffset) * 180.0 / PI;

        // Dead zone
        if (abs(gz) < GYRO_DEADZONE) gz = 0;

        heading += gz * dt;

        // Wrap 0-360
        while (heading >= 360) heading -= 360;
        while (heading < 0)    heading += 360;

        headingRad = heading * PI / 180.0;
    }

    bool isPathClear()    { return distFront > OBSTACLE_CLEAR; }
    bool isObstacleClose(){ return distFront < OBSTACLE_CLOSE; }

    int bestDirection() {
        if (distFront > OBSTACLE_CLEAR) return 0;
        if (distLeft > distRight && distLeft > OBSTACLE_SLOW) return -1;
        if (distRight > distLeft && distRight > OBSTACLE_SLOW) return 1;
        if (distLeft > distRight) return -1;
        return 1;
    }

    float getBatteryVoltage() {
        int raw = analogRead(A0);
        return (raw / 1023.0) * BATTERY_MAX_V;
    }

private:
    float readUltrasonic() {
        digitalWrite(US_TRIG, LOW);  delayMicroseconds(2);
        digitalWrite(US_TRIG, HIGH); delayMicroseconds(10);
        digitalWrite(US_TRIG, LOW);
        unsigned long duration = pulseIn(US_ECHO, HIGH, 30000);
        if (duration == 0) return 999;
        float distance = duration / 58.0;
        return (distance > 400) ? 999 : distance;
    }

    void initMPU() {
        if (!mpu.begin(0x68, &Wire)) {
            // MPU6050 not found - heading will stay at 0
            return;
        }
        mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
        mpu.setGyroRange(MPU6050_RANGE_250_DEG);
        mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

        // Calibrate gyro Z offset (must be still!)
        calibrateGyro();
    }

    void calibrateGyro() {
        float sum = 0;
        int count = 0;
        unsigned long start = millis();

        while (millis() - start < 2000) {
            sensors_event_t a, g, t;
            mpu.getEvent(&a, &g, &t);
            sum += g.gyro.z;
            count++;
            delay(10);
        }

        if (count > 0) {
            gyroZOffset = sum / count;
            gyroCalibrated = true;
        }
    }
};

#endif // SENSORS_H
