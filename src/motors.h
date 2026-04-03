#ifndef MOTORS_H
#define MOTORS_H

#include <Arduino.h>
#include "config.h"
#include "encoder.h"   // For Encoder::setDirection()

// DRV8833 dual H-bridge driver for 4WD chassis
// FIX BUG 3: Reports direction to encoder on every motor command.

class Motors {
public:
    void begin() {
        pinMode(MOTOR_L_IN1, OUTPUT);
        pinMode(MOTOR_L_IN2, OUTPUT);
        pinMode(MOTOR_R_IN1, OUTPUT);
        pinMode(MOTOR_R_IN2, OUTPUT);
        stop();
    }

    void forward(int speed = SPEED_MEDIUM) {
        Encoder::setDirection(1);  // Forward
        setLeft(speed);
        setRight(speed);
    }

    void backward(int speed = SPEED_MEDIUM) {
        Encoder::setDirection(-1); // Backward
        setLeft(-speed);
        setRight(-speed);
    }

    void turnLeft(int speed = SPEED_MEDIUM) {
        Encoder::setDirection(0);  // Spinning, distance not meaningful
        setLeft(-speed);
        setRight(speed);
    }

    void turnRight(int speed = SPEED_MEDIUM) {
        Encoder::setDirection(0);  // Spinning
        setLeft(speed);
        setRight(-speed);
    }

    void curveLeft(int speed = SPEED_MEDIUM) {
        Encoder::setDirection(1);  // Mostly forward
        setLeft(speed / 3);
        setRight(speed);
    }

    void curveRight(int speed = SPEED_MEDIUM) {
        Encoder::setDirection(1);  // Mostly forward
        setLeft(speed);
        setRight(speed / 3);
    }

    void brake() {
        Encoder::setDirection(0);
        digitalWrite(MOTOR_L_IN1, HIGH);
        digitalWrite(MOTOR_L_IN2, HIGH);
        digitalWrite(MOTOR_R_IN1, HIGH);
        digitalWrite(MOTOR_R_IN2, HIGH);
    }

    void stop() {
        Encoder::setDirection(0);
        analogWrite(MOTOR_L_IN1, 0);
        analogWrite(MOTOR_L_IN2, 0);
        analogWrite(MOTOR_R_IN1, 0);
        analogWrite(MOTOR_R_IN2, 0);
    }

private:
    void setLeft(int speed) {
        if (speed > 0) {
            analogWrite(MOTOR_L_IN1, speed);
            analogWrite(MOTOR_L_IN2, 0);
        } else if (speed < 0) {
            analogWrite(MOTOR_L_IN1, 0);
            analogWrite(MOTOR_L_IN2, -speed);
        } else {
            analogWrite(MOTOR_L_IN1, 0);
            analogWrite(MOTOR_L_IN2, 0);
        }
    }

    void setRight(int speed) {
        if (speed > 0) {
            analogWrite(MOTOR_R_IN1, speed);
            analogWrite(MOTOR_R_IN2, 0);
        } else if (speed < 0) {
            analogWrite(MOTOR_R_IN1, 0);
            analogWrite(MOTOR_R_IN2, -speed);
        } else {
            analogWrite(MOTOR_R_IN1, 0);
            analogWrite(MOTOR_R_IN2, 0);
        }
    }
};

#endif // MOTORS_H
