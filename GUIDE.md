# ESP8266 4WD Robot Chassis - Beginner Guide

## What Is This Project?

This is a self-driving robot car that:
- **Avoids obstacles** using an ultrasonic sensor
- **Finds its way** to a destination you set (pathfinding)
- **Reports everything** to a dashboard on your computer
- Can be **manually controlled** from your browser

You control it from a web dashboard running on your PC. Both the robot and your PC connect to your **phone's hotspot** (same WiFi network).

---

## How The System Works (Big Picture)

```
    Your Phone (Hotspot)
         │
    ┌────┴────┐
    │  WiFi   │
    ├─────────┤
    │         │
[Your PC]  [Robot ESP8266]
    │         │
    │ Flask   │ Reads sensors
    │ server  │ Drives motors
    │         │ Avoids obstacles
    │         │
    │ Every 200ms:
    │  ← Robot POSTs sensor data
    │  → Robot GETs commands
    │
    │ Every 500ms:
    │  Browser polls status
    │  Browser sends your clicks
```

The ESP8266 does NOT host a web page. Instead, it acts as a **client** that talks to your PC's server via the phone hotspot.

---

## Your Components

| Part | What It Does |
|------|-------------|
| **NodeMCU ESP8266** | The brain. Runs the code, has WiFi built-in |
| **DRV8833** | Motor driver chip. Takes signals from ESP8266, sends power to motors |
| **4WD Chassis + 4 Motors** | The frame and wheels. 4 motors, but wired as 2 pairs (left side + right side) |
| **HC-SR04 Ultrasonic** | Measures distance to objects (like a bat). Sends a sound pulse, measures echo time |
| **SG90 Servo** | A small motor that rotates the ultrasonic sensor left/center/right to scan |
| **MPU6050** | Gyroscope + accelerometer. Knows if the robot is tilting or rotating |
| **Compass Module** | Tells which direction is north. Used for navigation heading |
| **Wheel Encoder (x1)** | A slotted disc on one wheel. Counts rotations to measure distance traveled |
| **6V Battery** | Powers everything |

---

## How The Wiring Works

### Motor Wiring (DRV8833)

The DRV8833 has 2 channels. Each channel can drive motors forward or backward:

```
Channel A: Left Front Motor  ──┐
                                ├── wired in PARALLEL (same 2 wires)
           Left Rear Motor   ──┘

Channel B: Right Front Motor ──┐
                                ├── wired in PARALLEL (same 2 wires)
           Right Rear Motor  ──┘
```

"Parallel" means: connect both motor + wires together, both motor - wires together. They spin together as one.

**Why DRV8833 not L298N?** DRV8833 is smaller, more efficient (less heat), and works fine at 6V. L298N wastes ~2V as heat.

### How The DRV8833 Controls Direction

Each channel has 2 input pins (IN1, IN2):

| IN1 | IN2 | Motor Action |
|-----|-----|-------------|
| PWM | 0 | Forward (speed = PWM value) |
| 0 | PWM | Backward (speed = PWM value) |
| 0 | 0 | Coast (free spin, no power) |
| 1 | 1 | Brake (motor locked) |

PWM = Pulse Width Modulation. It's how we control speed. Value 0-1023 on ESP8266 (0 = stop, 1023 = full speed).

### I2C Bus (Shared Wire)

MPU6050 and Compass both connect to the **same 2 wires** (SDA + SCL). This is called I2C - a protocol where multiple devices share one wire pair. Each device has a unique address:
- MPU6050 address: `0x68`
- Compass address: `0x0D`

The ESP8266 talks to each one by calling its address, like calling different phone numbers on the same line.

### Ultrasonic Sensor (HC-SR04)

```
1. ESP8266 sends a 10 microsecond pulse on TRIGGER pin
2. HC-SR04 sends an ultrasonic sound wave
3. Sound bounces off obstacle and returns
4. HC-SR04 sets ECHO pin HIGH for the duration of the round trip
5. ESP8266 measures that time
6. Distance = time / 58 (gives centimeters)
```

Max range: ~400cm. If no echo returns, it times out (we return 999 = no obstacle).

### Wheel Encoder

A disc with slots attached to a wheel. A sensor counts the slots as they pass:
- 20 slots per full wheel rotation
- Wheel circumference = 20.4cm
- So each slot = ~1cm of distance
- Connected to GPIO3 (the RX pin) using an **interrupt** - the ESP8266 automatically counts every tick without slowing down the main code

**Important:** Serial (USB debug output) is disabled after boot because we need the RX pin for the encoder. Debug messages only appear during the first ~1 second of startup.

---

## Understanding The Code Files

### `config.h` - All Settings In One Place

Every pin number, speed value, threshold, and WiFi credential lives here. If you need to change anything (like your WiFi password or server IP), this is the only file to edit.

Key settings to change:
```cpp
#define WIFI_SSID     "YourPhoneHotspot"     // Phone hotspot name
#define WIFI_PASSWORD "YourHotspotPassword"  // Phone hotspot password
#define SERVER_HOST   "192.168.43.100"       // Your PC's IP on the hotspot
```

### `motors.h` - Moving The Robot

Simple commands:
- `motors.forward(speed)` - both sides forward
- `motors.backward(speed)` - both sides backward
- `motors.turnLeft(speed)` - left backward + right forward (spin turn)
- `motors.turnRight(speed)` - left forward + right backward
- `motors.curveLeft(speed)` - left slow + right fast (gentle turn)
- `motors.stop()` - coast to stop
- `motors.brake()` - lock wheels immediately

### `sensors.h` - Reading The World

The single ultrasonic sensor is mounted on a servo. To see left, center, and right:
1. Servo turns to 160 degrees (left), measure distance
2. Servo turns to 90 degrees (center), measure distance
3. Servo turns to 20 degrees (right), measure distance
4. Servo returns to center

This "sweep" takes ~450ms. For quick checks, `updateFrontOnly()` just reads center (~50ms).

The compass reads magnetic heading (0-360 degrees, where 0 = North).

### `encoder.h` - Tracking Distance

Counts wheel rotations using hardware interrupts. Every 100ms, it:
1. Reads how many ticks happened since last check
2. Converts ticks to centimeters
3. Uses compass heading to calculate X,Y position change
4. Adds to running totals

### `avoidance.h` - Don't Hit Things (Reactive Layer)

This is the **safety system**. It has priority over everything:

```
                    distFront > 50cm
                   ┌─────────────────────┐
                   │    IDLE             │ ← Pathfinder controls motors
                   └──────┬──────────────┘
                          │ distFront < 30cm
                   ┌──────▼──────────────┐
                   │    SLOWDOWN         │ ← Sweep scan, curve away
                   └──────┬──────────────┘
                          │ distFront < 15cm
                   ┌──────▼──────────────┐
                   │    SCANNING         │ ← Full stop, sweep all directions
                   └──────┬──────────────┘
                          │
                   ┌──────▼──────────────┐
                   │    REVERSING        │ ← Back up 300ms
                   └──────┬──────────────┘
                          │
                   ┌──────▼──────────────┐
                   │    TURNING          │ ← Turn toward clearest direction
                   └─────────────────────┘
```

### `pathfinder.h` - Finding The Way (Planning Layer)

Uses the **A* algorithm** on a **sliding** 20x20 grid with **unlimited range**.

How it works:
1. Robot starts at world position (0, 0) cm
2. You set a target in centimeters (e.g., X=0, Y=500 = 5 meters forward)
3. The grid is centered on the robot. As the robot moves, the grid slides with it
4. Ultrasonic readings mark cells as "obstacle" or "free"
5. A* finds the shortest path avoiding obstacles
6. Path recalculates every 500ms as new obstacles are discovered
7. If target is beyond current grid view, robot pathfinds toward the grid edge closest to the target, then the grid slides and it continues

The robot also drops **breadcrumbs** every 20cm. These are waypoints stored in memory that let it **backtrack** home by retracing its path in reverse.

### `robot_main.ino` - The Main Loop

This ties everything together. Every loop cycle:

```
Every 200ms: POST telemetry to server, GET commands from server
Every 100ms: Read compass, update encoder position, check for obstacles
Every 500ms: Recalculate A* path
```

Priority: **Avoidance > Pathfinding > Manual commands**

If the avoidance system detects a close obstacle, it overrides pathfinding AND manual control to keep the robot safe.

---

## The Server Side

### `server/app.py` - Flask Server

A lightweight Python web server that:
1. **Receives data** from the robot (POST /api/robot/report)
2. **Serves commands** to the robot (GET /api/robot/command)
3. **Serves the dashboard** webpage to your browser
4. **Relays your clicks** from the dashboard to the robot

It stores the latest robot state in memory (a Python dictionary). No database needed.

Command flow: You click "FWD" on dashboard -> browser sends POST to server -> server stores command -> robot fetches it on next GET -> robot executes.

### `server/templates/dashboard.html` - The Dashboard

Three panels:
1. **Status** - Live sensor readings, battery, heading, position, mode, breadcrumbs
2. **Destination** - Set target in cm, backtrack button, live grid map
3. **Controls** - Manual drive buttons + autonomous + backtrack + reset

Controls: **WASD** or **Arrow Keys** to drive, **Space** = stop, **B** = backtrack, **R** = reset.

---

## How Self-Tracking Works

This is the most important concept. The robot needs to know WHERE IT IS at all times, without GPS (GPS doesn't work indoors and is too inaccurate for a small robot).

### Dead Reckoning (Encoder + Compass)

The robot combines two sensors to track its position:

```
Step 1: Encoder counts wheel ticks
        "I moved 5 ticks = 5.1 cm"

Step 2: Compass reads heading
        "I'm facing 45 degrees (northeast)"

Step 3: Math converts distance + heading to X,Y change
        deltaX = 5.1 * sin(45°) = 3.6 cm east
        deltaY = 5.1 * cos(45°) = 3.6 cm north

Step 4: Add to running position
        posX = old_posX + 3.6
        posY = old_posY + 3.6
```

This runs every **100ms** (10 times per second). The position is in **centimeters** relative to where the robot started (the start position is always 0, 0).

### How Accurate Is It?

**Honestly: not perfect.** Here's why:

| Source of Error | Effect |
|----------------|--------|
| Only 1 encoder (not 2) | Can't detect if wheels slip on one side |
| Encoder has 20 slots | Resolution is ~1cm per tick (can't measure smaller movements) |
| Compass near motors | Motor magnets can shift compass readings by 5-15 degrees |
| Wheel slip on turns | Spinning turns slip more than straight driving |
| Accumulated drift | Small errors add up over distance |

**Expected accuracy:**
- Short distance (< 1m): within ~5-10cm of real position
- Medium distance (1-5m): within ~20-50cm
- Long distance (> 5m): could be off by 50-100cm+

**The robot considers the target "reached" when it gets within 15cm.** This is intentionally generous because the position tracking isn't precise enough for exact positioning.

### Is "Nearly There" Acceptable?

**Yes.** The system is designed for "close enough":
- Target reached = within **15cm** of the destination
- This accounts for sensor drift and wheel slip
- For a small robot navigating a room, 15cm accuracy is practical

If you need better accuracy, you could add:
- A second wheel encoder (measures turning more accurately)
- IR line following for precise paths
- Visual markers (camera-based, but ESP8266 can't do this)

---

## How The Sliding Grid Works

### The Problem
The pathfinding grid is 20x20 cells at 10cm each = 2m x 2m. But the robot might need to travel 5m or 10m.

### The Solution: Sliding Window

The grid is like a **window** that follows the robot. The robot is always near the center:

```
Time 0: Robot at start          Time 1: Robot moved right
┌──────────────────────┐       ┌──────────────────────┐
│          [R]         │       │                      │
│                      │  -->  │         [R]          │
│                      │       │                      │
│                      │       │                      │
└──────────────────────┘       └──────────────────────┘
Grid covers 0-200cm            Grid shifted! Now covers 40-240cm
                               Old left edge data discarded
                               New right edge filled with "unknown"
```

When the robot gets within **4 cells of any edge**, the entire grid shifts:
1. Known obstacle/free data shifts with it (preserved)
2. New cells at the edge are marked "unknown"
3. The robot is re-centered
4. Target position is recalculated on the new grid

**This means unlimited travel range** with only 400 bytes of grid memory.

### Navigating Beyond The Grid View

If your target is 5m away but the grid only shows 2m:
1. The pathfinder clamps the target to the nearest grid edge
2. Robot pathfinds to that edge (avoiding obstacles along the way)
3. Grid slides as robot moves
4. New obstacles are discovered and mapped
5. Pathfinder recalculates toward the (still distant) target
6. Repeat until target is within the grid view
7. Final approach to exact target position

---

## How Backtracking Works

The robot drops **breadcrumbs** as it drives - small waypoints stored every 20cm of travel. Up to 50 breadcrumbs are stored (covering ~10m of travel).

```
Start ──(crumb)──(crumb)──(crumb)──(crumb)──[Robot is here]
  0        20cm    40cm    60cm     80cm

When you press BACKTRACK:

Start ──(crumb)──(crumb)──(crumb)──[Robot]──(done)
  0        20cm    40cm    goes to    60cm
                           each crumb
                           in reverse
```

Backtracking follows breadcrumbs in **reverse order**, from newest to oldest. At each crumb, the robot uses A* pathfinding to navigate there (avoiding any obstacles discovered on the way back).

If more than 50 crumbs exist (> 10m of travel), the oldest crumbs are dropped. The robot can still backtrack to the most recent 10m of its path.

---

## How To Run It

### Step 1: Turn On Phone Hotspot

- **Android:** Settings > Connections > Mobile Hotspot and Tethering
- **iPhone:** Settings > Personal Hotspot
- Note the hotspot name and password
- **Important:** Must be 2.4GHz (ESP8266 doesn't support 5GHz)

### Step 2: Connect PC to Hotspot

Join the phone's WiFi from your laptop. Then find your PC's IP:
```bash
# Windows
ipconfig
# Look for "Wireless LAN adapter Wi-Fi" → IPv4 Address
# Android hotspot typically gives: 192.168.43.x
# iPhone hotspot typically gives: 172.20.10.x
```

### Step 3: Start the Server

```bash
cd robot_chassis/server
pip install flask
python app.py
```

Open browser: `http://localhost:5000`

### Step 4: Configure & Upload to ESP8266

Edit `src/config.h`:
```cpp
#define WIFI_SSID     "YourPhoneHotspot"
#define WIFI_PASSWORD "HotspotPassword"
#define SERVER_HOST   "192.168.43.100"  // Your PC's IP from Step 2
```

Upload:
```bash
cd robot_chassis
pio run -t upload
```

### Step 5: Power On & Drive

1. Power on the robot
2. Dashboard shows "CONNECTED" in green
3. Set destination in cm (e.g., X=0, Y=200 = 2 meters forward)
4. Press "Go To Target" - robot navigates autonomously
5. Press "Backtrack Home" to retrace its path back

---

## Dashboard Controls Reference

| Action | Button | Keyboard |
|--------|--------|----------|
| Drive forward | FWD | W or Arrow Up |
| Drive backward | BACK | S or Arrow Down |
| Turn left | LEFT | A or Arrow Left |
| Turn right | RIGHT | D or Arrow Right |
| Stop | STOP | Space |
| Autonomous mode | AUTONOMOUS | - |
| Backtrack home | BACKTRACK HOME | B |
| Reset position to 0,0 | RESET POSITION | R |
| Set destination | Go To Target | - |

---

## Troubleshooting

| Problem | Fix |
|---------|-----|
| Dashboard says DISCONNECTED | Check WiFi credentials in config.h. Check SERVER_HOST matches your PC's IP. Both must be on same hotspot |
| Motors don't move | Check DRV8833 wiring. Check battery charged. Check SLP pin connected to 3.3V |
| Distance always shows 999 | Check HC-SR04 wiring (Trigger=D0, Echo=D4). Needs 5V power |
| Robot spins in circles | Left/right motor wires swapped at DRV8833 |
| Robot drives backward | Swap (+) and (-) wires of BOTH motor channels |
| Heading is wrong | Compass too close to motors. Mount it higher/farther. Calibrate by rotating robot 360° |
| Encoder count stays 0 | Check encoder is on GPIO3 (RX pin). Serial disables after boot - this is normal |
| Robot misses target by a lot | Normal for dead reckoning. Compass drift + wheel slip cause position error |
| Hotspot won't connect | Must be 2.4GHz. Disable "auto-off" on hotspot. Double-check SSID spelling |
| Grid map doesn't update | Check that encoder ticks are counting (position must change) |
| Backtrack doesn't work | Need at least 20cm of travel to create first breadcrumb |

---

## Key Concepts For Beginners

### PWM (Pulse Width Modulation)
Instead of just ON/OFF, PWM rapidly switches a pin on and off. The ratio of on-time to off-time controls motor speed. 0 = always off, 1023 = always on, 512 = half speed.

### Interrupt
A hardware feature that runs a tiny function instantly when a pin changes state. Used for the encoder - the ESP8266 counts every slot tick even while doing other work. Without interrupts, we'd miss ticks.

### Dead Reckoning
Estimating your position by measuring how far you've moved and in what direction, starting from a known point. Like walking with your eyes closed: you count steps and remember which way you turned. It drifts over time because small errors accumulate.

### I2C Protocol
A 2-wire communication bus (SDA = data, SCL = clock). Multiple sensors share the same 2 wires. Each sensor has a unique address (like phone numbers on a shared line).

### A* Algorithm
A pathfinding algorithm used in games and robotics. Imagine a grid of rooms. Some rooms have walls (obstacles). A* explores rooms closest to the goal first, keeps track of the shortest path, and guarantees finding the optimal route.

### Sliding Window
A technique where you keep a fixed-size view that moves with the data. Instead of storing the entire world map (which would need too much memory), we only store what's around the robot right now. Old data behind us scrolls off; new data ahead scrolls in.

### Breadcrumb Trail
Inspired by Hansel and Gretel. The robot drops position markers as it moves. To go home, it follows the markers in reverse. Simple, reliable, and works even when the map has changed (new obstacles appeared).

### Client-Server Architecture
The robot is a **client** - it sends data to and receives commands from the server. Your browser is also a client. The Flask **server** sits in the middle, storing state and relaying messages. Both clients connect via the phone hotspot's WiFi network.
