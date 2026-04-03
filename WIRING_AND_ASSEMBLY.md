# Wiring & Assembly Guide (v2 - ESP32-S3)

## Assembly Order

```
[ ] 1.  Assemble 4WD chassis (bottom plate + 4 motors + wheels)
[ ] 2.  Wire left 2 motors in parallel
[ ] 3.  Wire right 2 motors in parallel
[ ] 4.  Mount top plate with standoffs
[ ] 5.  Solder balance wire to battery holder middle tab
[ ] 6.  Wire: Battery holder → 2S BMS → power rails
[ ] 7.  Wire: 2S USB-C charger → BMS charge pads
[ ] 8.  Mount buck converter, wire: BMS P+ → buck → 5V rail
[ ] 9.  Solder 1000µF capacitor across BMS P+/P-
[ ] 10. Place ESP32-S3 on breadboard
[ ] 11. Wire: Buck 5V → ESP32 5V pin
[ ] 12. Place TB6612FNG on breadboard
[ ] 13. Wire: TB6612 VM → 7.4V (BMS P+)
[ ] 14. Wire: TB6612 VCC → 3.3V (ESP32)
[ ] 15. Wire: TB6612 GND → common GND
[ ] 16. Wire: TB6612 STBY → GPIO 17
[ ] 17. Wire: TB6612 AIN1/AIN2/PWMA → GPIO 4/5/6
[ ] 18. Wire: TB6612 BIN1/BIN2/PWMB → GPIO 7/15/16
[ ] 19. Wire: TB6612 motor outputs → left and right motor pairs
[ ] 20. Mount servo + HC-SR04 on front edge
[ ] 21. Wire: Servo → 5V + GND + GPIO 9
[ ] 22. Wire: HC-SR04 → 5V + GND + GPIO 18 (TRIG) + GPIO 8 (ECHO)
[ ] 23. Wire: MPU6050 → 3.3V (direct ESP32 pin!) + GND + GPIO 11/12
[ ] 24. Mount encoder discs on left + right wheel shafts
[ ] 25. Wire: Left encoder → 3.3V + GND + GPIO 13
[ ] 26. Wire: Right encoder → 3.3V + GND + GPIO 14
[ ] 27. Wire: Battery divider (220K+33K) → GPIO 1
[ ] 28. Connect all GND wires to common ground rail
[ ] 29. Mount USB-C charger port accessible on chassis wall
[ ] 30. Zip-tie cables, double-check everything
[ ] 31. Upload code, test!
```

---

## Power System Wiring (Do This First)

### Step 1: Battery Holder Balance Wire

The 2-slot holder only has 2 wires (red +, black -). You need a 3rd wire from the **middle** where the two cells connect:

```
Inside the battery holder:

    Cell 1 (3.7V)     Cell 2 (3.7V)
    ┌──────────┐      ┌──────────┐
    │ (+)  (-) │──────│ (+)  (-) │
    └──┬────┬──┘  ↑   └──┬────┬──┘
       │    │     │      │    │
       │    └─────┘      │    │
       │   SOLDER A      │    │
      B+   WIRE HERE    │   B-
    (red)  = Bm         (black)
           (any color wire)
```

### Step 2: BMS Wiring

```
Battery Holder          2S BMS 16A
──────────────          ──────────
Red (+)   ──────────→   B+
Middle    ──────────→   Bm
Black (-) ──────────→   B-

BMS outputs:
  P+ ──→ Robot 7.4V power (protected)
  P- ──→ Robot GND (protected)
```

### Step 3: Charger Wiring

```
2S USB-C Charger Module
┌───────────────────┐
│ USB-C port        │  ← mount on chassis edge, accessible
│                   │
│ BAT+  ────────────┼──→ BMS P+ (or B+, check your module's manual)
│ BAT-  ────────────┼──→ BMS P- (or B-)
└───────────────────┘

NOTE: Some charger modules connect to B+/B- (before BMS),
      others to P+/P- (after BMS). Check your module's datasheet.
      If unsure, connect to P+/P- which is safer.
```

### Step 4: Power Distribution

```
BMS P+ (7.4V protected)
  │
  ├──→ TB6612 VM pin (motor power, direct 7.4V)
  │
  ├──→ Buck converter IN+ ──→ 5V OUT ──┬── ESP32 5V pin
  │                                     ├── HC-SR04 VCC
  │                                     └── Servo VCC (red wire)
  │
  ├──[220K resistor]──┬──[33K resistor]── GND
  │                   └── ESP32 GPIO 1 (battery voltage reading)
  │
  └──┤├── 1000µF capacitor ──┤├── GND
     (+)                     (-)
     (longer leg)            (shorter leg)

BMS P- ──→ Common GND rail on breadboard
```

---

## TB6612FNG Motor Driver

```
TB6612FNG Module:
┌─────────────────────────────────┐
│ VM   VCC  GND  AO1  AO2  BO1  BO2
│ STBY AIN1 AIN2 PWMA BIN1 BIN2 PWMB
└─────────────────────────────────┘

Wiring:
  VM   ← 7.4V (BMS P+)
  VCC  ← 3.3V (ESP32 3V3 pin)
  GND  ← Common GND
  STBY ← ESP32 GPIO 17

  AIN1 ← GPIO 4    ┐
  AIN2 ← GPIO 5    ├ Left motors
  PWMA ← GPIO 6    ┘

  BIN1 ← GPIO 7    ┐
  BIN2 ← GPIO 15   ├ Right motors
  PWMB ← GPIO 16   ┘

  AO1 ──→ Left front + rear motor (+)  ┐ wired in
  AO2 ──→ Left front + rear motor (-)  ┘ parallel

  BO1 ──→ Right front + rear motor (+) ┐ wired in
  BO2 ──→ Right front + rear motor (-) ┘ parallel
```

---

## HC-SR04 + Servo

```
HC-SR04:                    SG90 Servo:
  VCC  ← 5V (buck out)       Red    ← 5V (buck out)
  GND  ← Common GND          Brown  ← Common GND
  TRIG ← ESP32 GPIO 18       Orange ← ESP32 GPIO 9
  ECHO → ESP32 GPIO 8
```

ESP32 GPIO 8 is 5V tolerant for input, but if worried use a 1K+2K voltage divider on ECHO.

---

## MPU6050

```
  VCC ← 3.3V DIRECTLY from ESP32 3V3 pin (NOT breadboard rail!)
  GND ← Common GND
  SDA ← ESP32 GPIO 11
  SCL ← ESP32 GPIO 12
```

**IMPORTANT:** MPU6050 is sensitive to voltage noise. Connect VCC with a short wire directly to the ESP32's 3.3V output pin. The breadboard rail has too much resistance/noise for reliable I2C.

---

## Wheel Encoders

```
Left encoder:                Right encoder:
  VCC ← 3.3V                  VCC ← 3.3V
  GND ← Common GND            GND ← Common GND
  OUT → ESP32 GPIO 13         OUT → ESP32 GPIO 14
```

Mount encoder discs on one left wheel shaft and one right wheel shaft. Position sensor modules so they read the slots as wheels turn.

---

## Complete Wire-by-Wire Checklist

```
ESP32-S3 Pin     →  Destination
──────────────────────────────────────
5V pin           ←  Buck converter 5V output
GND              →  Common GND rail
3V3              →  MPU6050 VCC (direct!)
                 →  TB6612 VCC (logic)
                 →  Left encoder VCC
                 →  Right encoder VCC

GPIO 1           ←  Battery voltage divider (220K/33K tap)
GPIO 4           →  TB6612 AIN1
GPIO 5           →  TB6612 AIN2
GPIO 6           →  TB6612 PWMA
GPIO 7           →  TB6612 BIN1
GPIO 8           ←  HC-SR04 ECHO
GPIO 9           →  Servo signal (orange)
GPIO 11          ↔  MPU6050 SDA
GPIO 12          ↔  MPU6050 SCL
GPIO 13          ←  Left encoder OUT
GPIO 14          ←  Right encoder OUT
GPIO 15          →  TB6612 BIN2
GPIO 16          →  TB6612 PWMB
GPIO 17          →  TB6612 STBY
GPIO 18          →  HC-SR04 TRIG

Power:
  BMS P+ (7.4V)  →  TB6612 VM
  BMS P+ (7.4V)  →  Buck converter IN+
  Buck 5V OUT    →  ESP32 5V pin
  Buck 5V OUT    →  HC-SR04 VCC
  Buck 5V OUT    →  Servo red wire
  BMS P-         →  Buck GND + Common GND rail
```

---

## ESP32-S3 Servo Control Notes

ESP32 does NOT use the standard Arduino `Servo.h` library. It uses **ESP32Servo** which internally uses LEDC PWM channels.

```
Standard Arduino (AVR/ESP8266):
  #include <Servo.h>         ← does NOT work on ESP32
  servo.attach(pin);

ESP32:
  #include <ESP32Servo.h>    ← use this instead
  servo.setPeriodHertz(50);          // 50Hz = standard servo
  servo.attach(pin, 500, 2400);     // min/max pulse in µs
  servo.write(90);                   // 0-180 degrees

LEDC channel allocation in this project:
  Channel 0 = Left motor PWM   (5kHz, 10-bit)
  Channel 1 = Right motor PWM  (5kHz, 10-bit)
  Channel 2 = Servo             (50Hz, auto-assigned by ESP32Servo)
  Channels 3-7 = free
```

---

## First Test Sequence

After assembly, test each system individually:

```
1. Power test:    Plug battery → ESP32 LED lights up
2. WiFi test:     Serial monitor shows "Connected" + IP
3. Motor test:    Dashboard → test_motors (left/right/fwd/bwd)
4. Servo test:    Dashboard → test_servo (sweeps L/C/R)
5. Ultrasonic:    Dashboard → test_ultrasonic (put hand in front)
6. MPU6050:       Dashboard → test_mpu (check accel Z gravity)
7. Encoders:      Dashboard → test_encoder (push robot by hand)
8. Battery:       Dashboard → test_battery (voltage reading)
9. Full test:     Dashboard → RUN ALL TESTS
10. Auto mode:    Set target → watch it navigate
```

---

## Charging

```
To charge:
  1. Plug USB-C cable into charger port on chassis
  2. LED on charger module shows charging status
  3. Wait until LED changes (usually red → green)
  4. Unplug

Do NOT run the robot while charging.
BMS protects against overcharge (cuts off at 8.4V)
and over-discharge (cuts off at ~6.0V).
```
