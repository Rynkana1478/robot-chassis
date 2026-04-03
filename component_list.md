# Robot Chassis - Component List (v2 Final)

## 1. Core Electronics

| Component | Qty | Status |
|-----------|-----|--------|
| ESP32-S3 DevKitC-1 | 1 | Confirmed |
| TB6612FNG Motor Driver | 1 | Confirmed |

## 2. Power System

| Component | Qty | Status |
|-----------|-----|--------|
| 18650 Li-Ion Cells (3.7V) | 2 | Confirmed |
| 18650 Battery Holder (2-slot, series) | 1 | Confirmed |
| 2S BMS 16A Protection Board | 1 | Confirmed |
| 2S USB-C Charger Module (4A) | 1 | Confirmed |
| Buck Converter (7.4V → 5V) | 1 | Confirmed |

## 3. Motors & Drivetrain

| Component | Qty | Status |
|-----------|-----|--------|
| 4WD Smart Car Chassis + 4 TT motors + 4 wheels | 1 | Have |

## 4. Sensors

| Component | Qty | Status |
|-----------|-----|--------|
| HC-SR04 Ultrasonic Sensor | 1 | Have |
| SG90 Servo Motor | 1 | Have |
| MPU6050 (Gyro + Accelerometer) | 1 | Have |
| Speed Encoder Module | 2 | 1 have + 1 confirmed |

## 5. Hardware

| Component | Qty | Status |
|-----------|-----|--------|
| Breadboard (half-size) | 1 | Have |
| Jumper Wires | 30 | Have |
| Resistors (220K + 33K) for battery divider | 1 set | Have |
| 1000µF Capacitor | 1 | Have |
| Standoffs + Screws (M3) | 8 | Have |

---

## Pin Assignment (ESP32-S3 DevKitC-1)

| GPIO | Assignment | Notes |
|------|------------|-------|
| 4 | TB6612 AIN1 | Left motor direction 1 |
| 5 | TB6612 AIN2 | Left motor direction 2 |
| 6 | TB6612 PWMA | Left motor speed (LEDC ch0) |
| 7 | TB6612 BIN1 | Right motor direction 1 |
| 15 | TB6612 BIN2 | Right motor direction 2 |
| 16 | TB6612 PWMB | Right motor speed (LEDC ch1) |
| 17 | TB6612 STBY | Standby (HIGH = active) |
| 18 | HC-SR04 TRIG | Ultrasonic trigger |
| 8 | HC-SR04 ECHO | Ultrasonic echo |
| 9 | Servo Signal | SG90 sweep (LEDC ch2, 50Hz) |
| 11 | I2C SDA | MPU6050 |
| 12 | I2C SCL | MPU6050 |
| 13 | Encoder Left | Left wheel ticks (interrupt) |
| 14 | Encoder Right | Right wheel ticks (interrupt) |
| 1 | Battery ADC | Via 220K+33K voltage divider |

---

## Wiring

### Power System
```
                    2S USB-C Charger
                   ┌───────────────┐
Phone charger ─────┤ USB-C IN      │
                   │               │
                   │ BAT+ ─────────┼──┐
                   │ BAT- ─────────┼──┼──┐
                   └───────────────┘  │  │
                                      │  │
                    2S BMS 16A        │  │
                   ┌──────────────┐   │  │
         ┌─────────┤ B+           ├───┘  │
         │  ┌──────┤ Bm (balance) │      │
         │  │  ┌───┤ B-           ├──────┘
         │  │  │   │              │
         │  │  │   │ P+ ──────────┼──→ Robot 7.4V out
         │  │  │   │ P- ──────────┼──→ Robot GND
         │  │  │   └──────────────┘
         │  │  │
    Battery Holder (2S series)
    ┌────┴──┴──┴────┐
    │[Cell1][Cell2]  │
    │(+)  mid  (-)   │
    └────────────────┘
```

### Robot Power Distribution
```
BMS P+ (7.4V) ──┬── TB6612 VM (motor power, direct)
                │
                ├── [Buck 7.4V→5V] ──┬── ESP32-S3 5V pin
                │                    └── HC-SR04 VCC
                │
                └── [220K]──┬──[33K]── GND
                            └── ESP32 GPIO 1 (battery ADC)

BMS P- (GND) ───── Common GND (all devices)

ESP32 3.3V ──┬── MPU6050 VCC (direct, not breadboard rail)
             ├── TB6612 VCC (logic)
             ├── Encoder L VCC
             └── Encoder R VCC

┌──┤├──┐  1000µF capacitor across BMS P+ and P- (motor power smoothing)
```

### TB6612FNG
```
TB6612FNG        ESP32-S3              Motors
---------        --------              ------
VM          ←    7.4V (BMS P+)         Motor power
VCC         ←    3.3V (ESP32)          Logic power
GND         ←    Common GND
STBY        ←    GPIO 17               HIGH = active
AIN1        ←    GPIO 4                Left dir 1
AIN2        ←    GPIO 5                Left dir 2
PWMA        ←    GPIO 6                Left speed
BIN1        ←    GPIO 7                Right dir 1
BIN2        ←    GPIO 15               Right dir 2
PWMB        ←    GPIO 16               Right speed
AO1/AO2     →    Left front+rear motors (parallel)
BO1/BO2     →    Right front+rear motors (parallel)
```

### HC-SR04
```
VCC   ←  5V (from buck converter)
GND   ←  Common GND
TRIG  ←  ESP32 GPIO 18
ECHO  →  ESP32 GPIO 8
```

### SG90 Servo
```
Red (VCC)     ←  5V (from buck converter)
Brown (GND)   ←  Common GND
Orange (SIG)  ←  ESP32 GPIO 9 (LEDC 50Hz servo PWM)
```

### MPU6050
```
VCC  ←  3.3V (direct from ESP32 3.3V pin, NOT breadboard rail)
GND  ←  Common GND
SDA  ←  ESP32 GPIO 11
SCL  ←  ESP32 GPIO 12
```

### Wheel Encoders
```
Left Encoder:
  VCC  ←  3.3V
  GND  ←  Common GND
  OUT  →  ESP32 GPIO 13

Right Encoder:
  VCC  ←  3.3V
  GND  ←  Common GND
  OUT  →  ESP32 GPIO 14
```

### Battery Voltage Divider
```
BMS P+ (7.4V) ──[220K]──┬──[33K]── GND
                         │
                         └── ESP32 GPIO 1

At 8.4V (full):  8.4 × 33/(220+33) = 1.10V
At 6.0V (empty): 6.0 × 33/(220+33) = 0.78V
ESP32 ADC range: 0-3.3V (12-bit) → safe
```

### ESP32-S3 Servo Notes
```
ESP32 does NOT use the standard Arduino Servo library.
Uses ESP32Servo library which wraps LEDC PWM channels.

LEDC channel allocation:
  Channel 0 = Left motor PWM (5kHz, 10-bit)
  Channel 1 = Right motor PWM (5kHz, 10-bit)
  Channel 2 = Servo (50Hz, auto-assigned by ESP32Servo)

Servo pulse range: 500-2400 µs
  500µs  = 0 degrees
  1450µs = 90 degrees (center)
  2400µs = 180 degrees
```
