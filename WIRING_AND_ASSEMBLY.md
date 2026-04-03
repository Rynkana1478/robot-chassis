# Wiring Connection Map & Assembly Guide

## Overview

```
                    FRONT OF ROBOT
            ┌─────────────────────────┐
            │    [Servo + HC-SR04]    │
            │         ┌───┐           │
            │         │ U │  Ultrasonic│
            │         └─┬─┘           │
            │      [SG90 Servo]       │
            │                         │
  Left      │  ┌───────────────────┐  │      Right
  Motors    ─┤  │   [ESP8266]       │  ├─    Motors
  (2x)      │  │   NodeMCU         │  │     (2x)
            │  └───────────────────┘  │
            │                         │
            │  ┌───────┐ ┌─────────┐  │
            │  │DRV8833│ │MPU6050  │  │
            │  │       │ │+Compass │  │
            │  └───────┘ └─────────┘  │
            │                         │
            │   ┌─────────────────┐   │
            │   │  6V Battery     │   │
            │   └─────────────────┘   │
            │                         │
            │  [Encoder]  on one wheel│
            └─────────────────────────┘
                    BACK OF ROBOT
```

---

## Step-by-Step Assembly

### Step 1: Build the 4WD Chassis Frame

Your 4WD smart car kit comes with:
- Acrylic/plastic chassis plates (usually 2 layers)
- 4 TT gear motors
- 4 wheels
- Mounting screws and standoffs

```
Assembly order:
1. Mount 4 motors to the BOTTOM chassis plate
   - 2 motors on left side, 2 on right side
   - Motors face outward (shaft sticking out the sides)
   - Use the included brackets and screws

2. Attach wheels to motor shafts
   - Press-fit onto the D-shaped shaft

3. Add standoffs on top of bottom plate
   - These create space for the TOP plate

4. Attach TOP plate
   - This is where all electronics go
```

### Step 2: Wire the Motors (IMPORTANT)

Each side has 2 motors. Wire them **in parallel** (same direction):

```
LEFT SIDE (parallel wiring):
                    ┌── Left Front Motor ── (+) ──┐
   AOUT1 (Motor+) ─┤                              ├── connected together
                    └── Left Rear Motor ─── (+) ──┘

                    ┌── Left Front Motor ── (-) ──┐
   AOUT2 (Motor-) ─┤                              ├── connected together
                    └── Left Rear Motor ─── (-) ──┘


RIGHT SIDE (parallel wiring):
                    ┌── Right Front Motor ── (+) ──┐
   BOUT1 (Motor+) ─┤                               ├── connected together
                    └── Right Rear Motor ─── (+) ──┘

                    ┌── Right Front Motor ── (-) ──┐
   BOUT2 (Motor-) ─┤                               ├── connected together
                    └── Right Rear Motor ─── (-) ──┘
```

**How to wire in parallel:**
1. Strip the wire ends of both left motors
2. Twist the (+) wires of both motors together
3. Twist the (-) wires of both motors together
4. Solder or use a terminal block
5. You now have 2 wires going to the DRV8833 for each side

**Testing motor direction:**
After full assembly, if the robot drives backward when you press FWD,
swap the (+) and (-) wires of BOTH sides at the DRV8833.

### Step 3: Mount Electronics on Top Plate

Place components on the top chassis plate:

```
    ┌─────────────────────────────────────────┐
    │                                         │
    │  ┌──────────┐              [Servo+US]   │  FRONT
    │  │ ESP8266  │              mounted on    │
    │  │ NodeMCU  │              front edge    │
    │  │          │                            │
    │  └──────────┘                            │
    │                                         │
    │  ┌────────┐    ┌──────────────────┐      │
    │  │DRV8833 │    │  Breadboard      │      │
    │  │        │    │  (MPU6050 +      │      │
    │  └────────┘    │   Compass here)  │      │
    │                └──────────────────┘      │
    │                                         │
    │           ┌────────────────┐             │
    │           │  6V Battery    │             │  BACK
    │           └────────────────┘             │
    └─────────────────────────────────────────┘
```

Use standoffs, double-sided tape, or zip ties to secure everything.

### Step 4: Mount the Ultrasonic Sensor + Servo

```
Side view:

    HC-SR04 ultrasonic
    ┌──────────────┐
    │  ○        ○  │  <- two "eyes" (transmitter + receiver)
    │  Trig Echo   │
    │  VCC  GND    │
    └──────┬───────┘
           │  (glue, screw, or bracket)
    ┌──────┴───────┐
    │   SG90 Servo │  <- rotates left/right
    │    ┌───┐     │
    │    │   │     │
    └────┴───┴─────┘
         │
    mounted on front edge of chassis
```

1. Attach the servo horn (the cross-shaped piece) to the servo
2. Mount/glue HC-SR04 on top of the servo horn
3. Screw the servo body to the front of the chassis

### Step 5: Mount the Wheel Encoder

```
Encoder module placement (on ONE wheel):

    ┌─────────┐
    │ Encoder │  <- the sensor module (has LED + photodetector)
    │ Module  │
    └────┬────┘
         │  (mounted on chassis, facing the disc)
    ═════╪═════  <- encoder disc (slotted)
         │       mounted on the motor shaft
    ┌────┴────┐     (between motor and wheel)
    │  Motor  │
    └─────────┘
```

1. Attach the encoder disc to one motor shaft (before the wheel)
2. Mount the encoder sensor module on the chassis so it reads the disc slots
3. The sensor has 3 pins: VCC, GND, OUT

---

## Complete Wiring Connection Map

### Power Connections

```
6V Battery Pack
    │
    ├──(+)──→ DRV8833 VCC pin (motor power)
    ├──(+)──→ ESP8266 Vin pin (onboard regulator → 3.3V)
    │         NOTE: ESP8266 Vin accepts 5-9V, 6V is fine
    │
    └──(-)──→ COMMON GROUND (connect ALL grounds together)
              ├── DRV8833 GND
              ├── ESP8266 GND
              ├── HC-SR04 GND
              ├── SG90 Servo GND (brown wire)
              ├── MPU6050 GND
              ├── Compass GND
              └── Encoder GND
```

**CRITICAL: All GND pins must be connected together. Use breadboard ground rail.**

### DRV8833 Motor Driver Wiring

```
DRV8833 Module Pinout:
┌─────────────────────────┐
│  IN1  IN2  IN3  IN4     │  <- Control inputs (from ESP8266)
│                         │
│  OUT1 OUT2 OUT3 OUT4    │  <- Motor outputs
│                         │
│  VCC  GND  SLP          │  <- Power
└─────────────────────────┘

Connections:
┌──────────────┬────────────────────────────────┐
│ DRV8833 Pin  │ Connect To                     │
├──────────────┼────────────────────────────────┤
│ VCC          │ 6V Battery (+)                 │
│ GND          │ Common Ground                  │
│ SLP (sleep)  │ 3.3V (ESP8266 3V3) to enable  │
│ IN1 (AIN1)   │ ESP8266 D5 (GPIO14)           │
│ IN2 (AIN2)   │ ESP8266 D6 (GPIO12)           │
│ IN3 (BIN1)   │ ESP8266 D0 (GPIO16)           │
│ IN4 (BIN2)   │ ESP8266 D8 (GPIO15)           │
│ OUT1 (AOUT1) │ Left motors (+) wire          │
│ OUT2 (AOUT2) │ Left motors (-) wire          │
│ OUT3 (BOUT1) │ Right motors (+) wire         │
│ OUT4 (BOUT2) │ Right motors (-) wire         │
└──────────────┴────────────────────────────────┘
```

**SLP pin:** The DRV8833 has a sleep pin. Connect it to 3.3V to keep the
driver always active. If left floating, the driver may not work.

### HC-SR04 Ultrasonic Sensor Wiring

```
HC-SR04 Pinout:
┌──────────────────┐
│  VCC TRIG ECHO GND│
└──┬───┬────┬───┬──┘
   │   │    │   │
   │   │    │   └── Common GND
   │   │    └────── ESP8266 D7 (GPIO13)
   │   └─────────── ESP8266 D4 (GPIO2)
   └─────────────── 5V (from ESP8266 Vin or Battery)
```

**NOTE:** HC-SR04 needs 5V to work. The ESP8266's Vin pin outputs the
input voltage (6V from battery) - use that, or tap 5V from the DRV8833's
VCC line. The Echo pin outputs 5V but ESP8266 GPIO13 is 5V-tolerant in
most NodeMCU boards. If worried, use a 1K+2K voltage divider on Echo.

**WHY NOT D0 FOR TRIGGER?** GPIO16 (D0) is connected to the ESP8266's
RTC module and has different output timing. The 10-microsecond trigger
pulse doesn't work reliably on this pin. GPIO2 (D4) works correctly.

### SG90 Servo Wiring

```
Servo has 3 wires:
┌────────┬──────────────────────────┐
│ Wire   │ Connect To               │
├────────┼──────────────────────────┤
│ Brown  │ Common GND               │
│ Red    │ 5V (from Vin or DRV8833) │
│ Orange │ ESP8266 D3 (GPIO0)       │
└────────┴──────────────────────────┘
```

### I2C Bus (MPU6050 + Compass Module)

Both sensors share the same 2 data wires:

```
                    ┌───────────┐     ┌───────────┐
                    │  MPU6050  │     │  Compass  │
                    │           │     │ QMC5883L  │
                    │ VCC ──────┤     │ VCC ──────┤
ESP8266 3V3 ────────┤           ├─────┤           │
                    │ GND ──────┤     │ GND ──────┤
Common GND ─────────┤           ├─────┤           │
                    │ SDA ──────┤     │ SDA ──────┤
ESP8266 D2 (GPIO4)──┤           ├─────┤           │
                    │ SCL ──────┤     │ SCL ──────┤
ESP8266 D1 (GPIO5)──┤           ├─────┤           │
                    └───────────┘     └───────────┘

Both devices on the same SDA+SCL wires (I2C bus).
They have different addresses so they don't conflict.
```

**MPU6050 placement:** Mount FLAT on the chassis, away from motors.
The chip needs to be level to give accurate readings.

**Compass placement:** Mount as FAR from motors and battery as possible.
Motors create magnetic fields that interfere with compass readings.
Best spot: top of a standoff, elevated above the chassis.

### Wheel Encoder Wiring

```
Encoder Module (3 pins):
┌──────────┬────────────────────────────┐
│ Pin      │ Connect To                  │
├──────────┼────────────────────────────┤
│ VCC      │ 3.3V (from ESP8266 3V3)    │
│ GND      │ Common GND                 │
│ OUT (DO) │ ESP8266 RX (GPIO3)         │
└──────────┴────────────────────────────┘
```

**WARNING:** After the ESP8266 boots up, it disables Serial (UART) to
free the RX pin for the encoder. This means you CANNOT use Serial
Monitor after the first ~1 second of startup.

### Battery Monitor (Optional)

```
To read battery voltage on A0, you need a voltage divider
because A0 only accepts 0-1.0V:

Battery (+) ──── [100K resistor] ──┬── [20K resistor] ──── GND
                                   │
                                   └── ESP8266 A0

This divides 6V down to ~1.0V, safe for A0.
Without this, A0 will read max value always.
```

---

## Complete Wire-by-Wire Checklist

Check off each connection as you make it:

```
ESP8266 Pin      →  Destination
────────────────────────────────────
3V3              →  MPU6050 VCC
                 →  Compass VCC
                 →  DRV8833 SLP (sleep/enable)
                 →  Encoder VCC

GND              →  ALL other GND pins (use breadboard rail)

Vin              →  Battery (+) 6V
                 →  HC-SR04 VCC (needs 5V, 6V ok)
                 →  Servo Red wire

D0 (GPIO16)      →  DRV8833 IN3 (BIN1) - Right motors forward
D1 (GPIO5)       →  MPU6050 SCL + Compass SCL
D2 (GPIO4)       →  MPU6050 SDA + Compass SDA
D3 (GPIO0)       →  Servo Orange wire (signal)
D4 (GPIO2)       →  HC-SR04 TRIG
D5 (GPIO14)      →  DRV8833 IN1 (AIN1) - Left motors forward
D6 (GPIO12)      →  DRV8833 IN2 (AIN2) - Left motors backward
D7 (GPIO13)      →  HC-SR04 ECHO
D8 (GPIO15)      →  DRV8833 IN4 (BIN2) - Right motors backward
RX (GPIO3)       →  Encoder OUT
A0               →  Voltage divider center tap (optional)

DRV8833 VCC      →  Battery (+) 6V
DRV8833 OUT1     →  Left motors (+)
DRV8833 OUT2     →  Left motors (-)
DRV8833 OUT3     →  Right motors (+)
DRV8833 OUT4     →  Right motors (-)

Battery (-)      →  Common GND rail
```

---

## Network Setup (Phone Hotspot)

```
Your Phone (Hotspot)
    │
    ├── WiFi ── ESP8266 Robot (client, posts data)
    │
    └── WiFi ── Your PC/Laptop (runs Flask server)
                 │
                 └── Browser → http://localhost:5000
```

### How to Set Up

1. **Turn on phone hotspot**
   - Android: Settings > Connections > Mobile Hotspot
   - iPhone: Settings > Personal Hotspot
   - Note the hotspot name and password

2. **Connect your PC to the hotspot**
   - Join the hotspot WiFi from your laptop/PC
   - Find your PC's IP address:
     ```
     Windows:  ipconfig → look for "Wireless LAN" IPv4 Address
     Mac:      ifconfig en0 → look for "inet"
     Linux:    ip addr → look for wlan
     ```
   - Typical hotspot IPs: 192.168.43.x (Android) or 172.20.10.x (iPhone)

3. **Edit config.h on ESP8266**
   ```cpp
   #define WIFI_SSID     "YourPhoneHotspot"
   #define WIFI_PASSWORD "HotspotPassword"
   #define SERVER_HOST   "192.168.43.100"  // YOUR PC's IP from step 2
   ```

4. **Start the server on your PC**
   ```bash
   cd robot_chassis/server
   pip install flask
   python app.py
   ```

5. **Upload code to ESP8266**
   ```bash
   cd robot_chassis
   pio run -t upload
   ```

6. **Open dashboard**
   - Browser → http://localhost:5000
   - Should show CONNECTED when robot powers on

### Troubleshooting Network

| Problem | Fix |
|---------|-----|
| Robot won't connect to hotspot | Double-check SSID/password in config.h. Hotspot must be 2.4GHz (ESP8266 doesn't support 5GHz) |
| PC can't find its IP | Make sure PC is connected to the SAME hotspot |
| Dashboard says DISCONNECTED | Check SERVER_HOST in config.h matches PC's IP. Make sure Flask server is running |
| Connection drops randomly | Phone may turn off hotspot after idle. Disable auto-off in hotspot settings |
| Very slow/laggy | Phone hotspot bandwidth is shared. Close other apps using data |

---

## Assembly Checklist (Order of Operations)

```
[ ] 1. Assemble chassis frame (bottom plate + motors + wheels)
[ ] 2. Wire left 2 motors in parallel (twist + solder/crimp)
[ ] 3. Wire right 2 motors in parallel
[ ] 4. Mount top plate with standoffs
[ ] 5. Place ESP8266 on top plate (header pins into breadboard or tape down)
[ ] 6. Place DRV8833 on breadboard
[ ] 7. Connect DRV8833 → motors (4 wires, 2 per side)
[ ] 8. Connect ESP8266 → DRV8833 control pins (4 wires: D5-D8)
[ ] 9. Mount servo on front edge
[ ] 10. Attach HC-SR04 on servo horn
[ ] 11. Wire servo (3 wires: GND, 5V, D3)
[ ] 12. Wire HC-SR04 (4 wires: GND, 5V, D0, D4)
[ ] 13. Place MPU6050 on breadboard (flat and level)
[ ] 14. Place compass on breadboard (away from motors)
[ ] 15. Wire I2C bus: SDA chain (D2), SCL chain (D1), VCC (3V3), GND
[ ] 16. Mount encoder disc on one motor shaft
[ ] 17. Mount encoder sensor module facing the disc
[ ] 18. Wire encoder (3 wires: GND, 3V3, RX)
[ ] 19. Connect all GND wires to common ground rail
[ ] 20. Connect battery (+) to DRV8833 VCC and ESP8266 Vin
[ ] 21. Connect battery (-) to ground rail
[ ] 22. (Optional) Add voltage divider for A0 battery monitor
[ ] 23. Double-check ALL connections against the checklist above
[ ] 24. Zip-tie loose wires, secure everything
[ ] 25. Upload code, connect hotspot, test!
```

## First Test Sequence

After assembly, test each subsystem one at a time:

1. **Power test:** Connect battery. ESP8266 LED should light up.
2. **WiFi test:** Check Serial Monitor (115200 baud) for "Connected" message.
3. **Motor test:** Use dashboard manual controls. FWD should go forward.
4. **Servo test:** The ultrasonic sensor should sweep left-center-right.
5. **Ultrasonic test:** Put your hand in front. Dashboard "Front" value should change.
6. **Compass test:** Rotate robot. Dashboard "Heading" should change.
7. **Encoder test:** Push robot forward by hand. Dashboard "Encoder" ticks should count up.
8. **Auto test:** Press AUTONOMOUS. Robot should drive and avoid obstacles.
