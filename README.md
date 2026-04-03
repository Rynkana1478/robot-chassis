# ESP8266 4WD Robot Chassis

Autonomous 4WD robot with obstacle avoidance, A* pathfinding, and web dashboard control via phone hotspot.

## Features

- **Obstacle avoidance** - Ultrasonic sensor on servo sweep, non-blocking safety system
- **A* pathfinding** - Sliding 20x20 grid with unlimited range
- **Backtracking** - Breadcrumb trail to retrace path home
- **Web dashboard** - Live sensor data, manual controls, destination input, grid map
- **Phone hotspot LAN** - ESP8266 + PC both connect to phone hotspot

## Hardware

| Component | Purpose |
|-----------|---------|
| NodeMCU ESP8266 | Main controller (WiFi built-in) |
| DRV8833 | Dual H-bridge motor driver |
| 4WD Smart Car Chassis | Frame + 4 TT motors + wheels |
| HC-SR04 | Ultrasonic distance sensor |
| SG90 Servo | Sweeps ultrasonic left/center/right |
| MPU6050 | Gyroscope + accelerometer |
| Compass (QMC5883L) | Magnetic heading |
| Speed Encoder (x1) | Wheel odometry |
| 6V Battery | Power supply |

## Pin Wiring

| ESP8266 Pin | Connection |
|-------------|------------|
| D0 (GPIO16) | DRV8833 BIN1 (right motors fwd) |
| D1 (GPIO5) | I2C SCL |
| D2 (GPIO4) | I2C SDA |
| D3 (GPIO0) | Servo signal |
| D4 (GPIO2) | HC-SR04 Trigger |
| D5 (GPIO14) | DRV8833 AIN1 (left motors fwd) |
| D6 (GPIO12) | DRV8833 AIN2 (left motors bwd) |
| D7 (GPIO13) | HC-SR04 Echo |
| D8 (GPIO15) | DRV8833 BIN2 (right motors bwd) |
| RX (GPIO3) | Wheel encoder |
| A0 | Battery voltage (via divider) |

## Quick Setup

### Prerequisites

- [Python 3](https://www.python.org/downloads/) (for the dashboard server)
- [PlatformIO CLI](https://platformio.org/install/cli) or [VS Code + PlatformIO extension](https://platformio.org/install/ide?install=vscode)
- Phone with hotspot capability

### 1. Clone & Install

```bash
git clone https://github.com/YOUR_USERNAME/robot-chassis.git
cd robot-chassis

# Install Python server dependencies
cd server
pip install -r requirements.txt
cd ..
```

### 2. Connect to Phone Hotspot

1. Turn on phone hotspot (must be **2.4GHz**, ESP8266 doesn't support 5GHz)
2. Connect your PC to the hotspot
3. Find your PC's IP:
   ```bash
   # Windows
   ipconfig
   # Look for "Wireless LAN" → IPv4 Address
   # Android hotspot: typically 192.168.43.x
   # iPhone hotspot: typically 172.20.10.x
   ```

### 3. Configure ESP8266

Edit `src/config.h`:

```cpp
#define WIFI_SSID     "YourPhoneHotspot"
#define WIFI_PASSWORD "HotspotPassword"
#define SERVER_HOST   "192.168.43.100"  // Your PC's IP from step 2
```

### 4. Upload Firmware

```bash
# PlatformIO CLI
pio run -t upload

# Or use VS Code PlatformIO: click Upload button
```

### 5. Start Dashboard

```bash
cd server
python app.py
```

Open browser: **http://localhost:5000**

### 6. Power On Robot

Dashboard should show **CONNECTED** in green. You're ready to drive.

## Dashboard Controls

| Action | Button | Keyboard |
|--------|--------|----------|
| Drive | FWD/BACK/LEFT/RIGHT | WASD or Arrow Keys |
| Stop | STOP | Space |
| Autonomous mode | AUTONOMOUS | - |
| Return to start | BACKTRACK HOME | B |
| Reset position | RESET | R |
| Set destination | Type X,Y in cm → Go To Target | - |

## Project Structure

```
robot_chassis/
├── src/                        # ESP8266 firmware
│   ├── config.h                # Pin assignments, thresholds, WiFi config
│   ├── encoder.h               # Wheel encoder with direction tracking
│   ├── motors.h                # DRV8833 4WD motor control
│   ├── sensors.h               # Non-blocking ultrasonic sweep + compass
│   ├── avoidance.h             # Reactive obstacle avoidance state machine
│   ├── pathfinder.h            # A* pathfinding on sliding grid + backtrack
│   └── robot_main.cpp          # Main loop, WiFi client, telemetry
├── server/                     # PC-side dashboard server
│   ├── app.py                  # Flask REST API
│   ├── requirements.txt        # Python dependencies
│   └── templates/
│       └── dashboard.html      # Web dashboard UI
├── platformio.ini              # Build configuration
├── component_list.md           # Full BOM + wiring diagrams
├── WIRING_AND_ASSEMBLY.md      # Step-by-step assembly + wire checklist
└── GUIDE.md                    # Beginner-friendly explanation of everything
```

## Architecture

```
Phone Hotspot (2.4GHz WiFi)
    ├── ESP8266 Robot (client)
    │     ├── Every 100ms: read sensors, avoid obstacles, follow path
    │     └── Every 200ms: POST telemetry, GET commands
    │
    └── PC (Flask server)
          ├── Stores robot state
          ├── Serves web dashboard
          └── Queues user commands

Priority: Safety (avoidance) > Navigation (pathfinding) > Manual control
```

## Documentation

- **[GUIDE.md](GUIDE.md)** - How everything works (concepts, algorithms, accuracy)
- **[WIRING_AND_ASSEMBLY.md](WIRING_AND_ASSEMBLY.md)** - Build instructions with diagrams
- **[component_list.md](component_list.md)** - Parts list and pin assignments
