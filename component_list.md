# Robot Chassis - Component List (v2 - ESP32-S3 Upgrade)

## 1. Core Electronics

| Component | Qty | Price | Purpose |
|-----------|-----|-------|---------|
| ESP32-S3 DevKitC-1 | 1 | ~$15 | Dual-core processor, WiFi+BLE, 34 GPIO |
| TB6612FNG Motor Driver | 1 | ~$2 | Dual H-bridge, 1.2A/ch, separate PWM+DIR |
| 2S 18650 Battery Holder | 1 | ~$0.50 | 7.4V power |
| 18650 Li-Ion Cells (3.7V) | 2 | ~$3 | Rechargeable, high current |

## 2. Motors & Drivetrain

| Component | Qty | Price | Purpose |
|-----------|-----|-------|---------|
| 4WD Smart Car Chassis | 1 | (have) | Frame + 4 TT motors + 4 wheels |

## 3. Sensors

| Component | Qty | Price | Purpose |
|-----------|-----|-------|---------|
| HC-SR04 Ultrasonic | 1 | (have) | Obstacle distance |
| SG90 Servo | 1 | (have) | Sweeps ultrasonic L/C/R |
| MPU6050 | 1 | (have) | Gyro (fast heading) + Accel (tilt) |
| QMC5883L Compass (0x0D) | 1 | ~$1.50 | Magnetic heading (drift correction) |
| Speed Encoder Module | 2 | ~$1.50 | Left + right wheel odometry |

## 4. Hardware

| Component | Qty | Price | Purpose |
|-----------|-----|-------|---------|
| Breadboard (half-size) | 1 | (have) | Prototyping |
| Jumper Wires | 30 | (have) | Wiring |
| Standoffs + Screws (M3) | 8 | (have) | Mounting |

**Total: ~$23.50 / Budget: $25**

---

## Pin Assignment (ESP32-S3 DevKitC-1)

| GPIO | Assignment | Notes |
|------|------------|-------|
| 4 | TB6612 AIN1 | Left motor direction 1 |
| 5 | TB6612 AIN2 | Left motor direction 2 |
| 6 | TB6612 PWMA | Left motor speed (LEDC) |
| 7 | TB6612 BIN1 | Right motor direction 1 |
| 15 | TB6612 BIN2 | Right motor direction 2 |
| 16 | TB6612 PWMB | Right motor speed (LEDC) |
| 17 | TB6612 STBY | Standby (HIGH=active) |
| 18 | HC-SR04 TRIG | Ultrasonic trigger |
| 8 | HC-SR04 ECHO | Ultrasonic echo |
| 9 | Servo Signal | SG90 sweep |
| 11 | I2C SDA | MPU6050 + QMC5883L |
| 12 | I2C SCL | MPU6050 + QMC5883L |
| 13 | Encoder Left | Left wheel ticks |
| 14 | Encoder Right | Right wheel ticks |
| 1 | Battery ADC | Via voltage divider (ADC1 channel) |

### TB6612FNG Wiring
```
TB6612FNG        ESP32-S3         Motors
---------        --------         ------
VCC (VM)    ←    7.4V Battery     Motor power
VCC (logic) ←    3.3V             Logic power
GND         ←    Common GND
STBY        ←    GPIO 17          HIGH = active
AIN1        ←    GPIO 4           Left direction
AIN2        ←    GPIO 5
PWMA        ←    GPIO 6           Left speed (PWM)
BIN1        ←    GPIO 7           Right direction
BIN2        ←    GPIO 15
PWMB        ←    GPIO 16          Right speed (PWM)
AO1         →    Left front + rear motor (+)
AO2         →    Left front + rear motor (-)
BO1         →    Right front + rear motor (+)
BO2         →    Right front + rear motor (-)
```

### Power
```
7.4V (2S 18650)
  ├── TB6612 VM (motor power, max 13.5V)
  ├── ESP32-S3 Vin (onboard regulator → 3.3V)
  └── Battery divider → GPIO 1 (220K + 33K → max 0.96V at 8.4V)
GND = all common
```
