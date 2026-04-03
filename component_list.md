# Robotic Chassis - Component List
## Processor: NodeMCU ESP8266 (ESP-12E)

---

## 1. Core Electronics

| Component | Qty | Purpose |
|-----------|-----|---------|
| NodeMCU ESP8266 (ESP-12E) | 1 | Main processor, WiFi-enabled |
| DRV8833 Motor Driver Module | 1 | Dual H-bridge, drives 2 motor channels |
| 6V Battery Pack | 1 | Power supply for motors and logic |

## 2. Motors & Drivetrain

| Component | Qty | Purpose |
|-----------|-----|---------|
| 4WD Smart Car Chassis Frame | 1 | Includes 4x TT gear motors + 4x wheels |
| TT Gear Motor (3-6V DC) | 4 | Included with chassis (left pair + right pair) |
| Rubber Wheel (65mm) | 4 | Included with chassis |

**Wiring:** Left-side 2 motors wired in parallel to DRV8833 Channel A.
Right-side 2 motors wired in parallel to DRV8833 Channel B.

## 3. Sensors & Encoders

| Component | Qty | Purpose |
|-----------|-----|---------|
| HC-SR04 Ultrasonic Sensor | 1 | Front-mounted obstacle detection |
| SG90 Servo Motor | 1 | Sweeps ultrasonic sensor left/center/right |
| MPU6050 (Gyro + Accelerometer) | 1 | Orientation and heading tracking |
| Compass Module (HMC5883L/QMC5883L) | 1 | Magnetic heading for pathfinding |
| Speed Encoder Module (slot/disc) | 1 | Wheel odometry for position tracking |

## 4. Hardware

| Component | Qty | Purpose |
|-----------|-----|---------|
| Breadboard (half-size) | 1 | Prototyping connections |
| Jumper Wires (M-M, M-F, F-F) | 30 | Wiring |
| Standoffs + Screws (M3) | 8 | Sensor/board mounting |

---

## Pin Assignment (ESP8266 NodeMCU)

| GPIO Pin | Assignment |
|----------|------------|
| D1 (GPIO5) | I2C SCL (MPU6050, Compass) |
| D2 (GPIO4) | I2C SDA (MPU6050, Compass) |
| D3 (GPIO0) | Servo Signal (ultrasonic sweep) |
| D4 (GPIO2) | Ultrasonic Trigger |
| D7 (GPIO13) | Ultrasonic Echo |
| D5 (GPIO14) | DRV8833 AIN1 (Left motors forward) |
| D6 (GPIO12) | DRV8833 AIN2 (Left motors backward) |
| D0 (GPIO16) | DRV8833 BIN1 (Right motors forward) |
| D8 (GPIO15) | DRV8833 BIN2 (Right motors backward) |
| RX (GPIO3) | Wheel Encoder (Serial disabled after boot) |
| A0 | Battery voltage monitor (via voltage divider) |

## Wiring Diagram Notes

### DRV8833 Connections
```
DRV8833 Pin    ->  Connection
-----------        ----------
VCC            ->  6V Battery +
GND            ->  Battery GND + ESP GND (common ground)
AIN1           ->  ESP D5 (GPIO14)
AIN2           ->  ESP D6 (GPIO12)
BIN1           ->  ESP D0 (GPIO16)
BIN2           ->  ESP D8 (GPIO15)
AOUT1          ->  Left Front Motor +
AOUT2          ->  Left Front Motor - (parallel with Left Rear Motor)
BOUT1          ->  Right Front Motor +
BOUT2          ->  Right Front Motor - (parallel with Right Rear Motor)
```

### I2C Bus (shared)
```
MPU6050 SDA  ->  ESP D2 (GPIO4)
MPU6050 SCL  ->  ESP D1 (GPIO5)
Compass SDA  ->  ESP D2 (GPIO4)  (same bus)
Compass SCL  ->  ESP D1 (GPIO5)  (same bus)
MPU6050 VCC  ->  3.3V from ESP
Compass VCC  ->  3.3V from ESP
```

### Power
```
6V Battery + -> DRV8833 VCC (motors)
6V Battery + -> ESP Vin (onboard regulator to 3.3V)
6V Battery - -> Common GND
```
