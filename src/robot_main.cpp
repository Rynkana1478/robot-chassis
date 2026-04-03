// ============================================
// ESP8266 4WD Robot Chassis
// DRV8833 | Ultrasonic + Servo Sweep
// Single Wheel Encoder | MPU6050 Gyro Heading
// Obstacle Avoidance + A* Pathfinding
// WiFi Debug Logger + Hardware Tests
// ============================================

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>

#include "config.h"
#include "encoder.h"
#include "motors.h"
#include "sensors.h"
#include "avoidance.h"
#include "pathfinder.h"
#include "debug.h"

// --- Global Objects ---
Motors            motors;
Sensors           sensors;
Encoder           encoder;
ObstacleAvoidance avoidance;
Pathfinder        pathfinder;
WiFiClient        wifiClient;

// --- Timing ---
unsigned long lastScan   = 0;
unsigned long lastPath   = 0;
unsigned long lastReport = 0;
unsigned long lastDebug  = 0;

// --- Robot State ---
bool autonomousMode = true;
bool emergencyBrake = false;

// --- Forward Declarations ---
void postTelemetry();
void fetchCommand();
void navigateWithPath();
void updateMap();
void setupWiFi();
void runTest(const char* test);
void debugFlush();
void testI2C();
void testUltrasonic();
void testServo();

void testMPU6050();
void testMotors();
void testEncoder();
void testBattery();
void testAll();

// ============================================
// SETUP
// ============================================
void setup() {
    Serial.begin(115200);
    Serial.println("\n=== ESP8266 4WD Robot Starting ===");

    Wire.begin(I2C_SDA, I2C_SCL);

    motors.begin();
    sensors.begin();
    avoidance.begin();
    pathfinder.begin();
    Debug::begin();

    setupWiFi();

    #if DEBUG_MODE
        Serial.println("[DEBUG] Debug mode ON - Serial stays active");
        Serial.println("[DEBUG] Encoder DISABLED (RX pin used by Serial)");
        Debug::log("Boot: Debug mode ON, encoder disabled");
    #else
        Serial.println("=== Robot Ready ===");
        Serial.println("Disabling Serial for encoder pin...");
        delay(100);
        encoder.begin();
        Debug::log("Boot: Normal mode, encoder active");
    #endif
}

// ============================================
// MAIN LOOP
// ============================================
void loop() {
    unsigned long now = millis();

    // --- Sweep tick ---
    sensors.sweepTick();

    // --- Safety: always check front ---
    if (sensors.readFrontNow() && sensors.distFront < OBSTACLE_CLOSE) {
        if (!autonomousMode) {
            motors.brake();
            emergencyBrake = true;
            Debug::logf("[SAFETY] Emergency brake! dist=%.0f cm", sensors.distFront);
        }
    } else if (emergencyBrake && sensors.servoAtCenter() && sensors.distFront > OBSTACLE_SLOW) {
        emergencyBrake = false;
        Debug::log("[SAFETY] Emergency brake cleared");
    }

    // --- Report telemetry + fetch commands (every 200ms) ---
    if (now - lastReport >= REPORT_INTERVAL_MS) {
        lastReport = now;
        if (WiFi.status() == WL_CONNECTED) {
            postTelemetry();
            if (!emergencyBrake) {
                fetchCommand();
            }
        } else {
            // Try to reconnect
            static unsigned long lastReconnect = 0;
            if (now - lastReconnect > 5000) {
                lastReconnect = now;
                Debug::log("[WIFI] Disconnected, reconnecting...");
                WiFi.reconnect();
            }
        }
    }

    // --- Flush debug logs to server (every 500ms) ---
    if (now - lastDebug >= 500) {
        lastDebug = now;
        if (WiFi.status() == WL_CONNECTED) {
            debugFlush();
        }
    }

    #if DEBUG_MODE
        // In debug mode, also print to Serial
        static unsigned long lastSerial = 0;
        if (now - lastSerial >= 1000) {
            lastSerial = now;
            Serial.printf("[DBG] F:%.0f L:%.0f R:%.0f H:%.0f Enc:%ld Batt:%.1fV State:%d\n",
                sensors.distFront, sensors.distLeft, sensors.distRight,
                sensors.heading, encoder.getCount(),
                sensors.getBatteryVoltage(), (int)avoidance.state);
        }
    #endif

    if (!autonomousMode) return;

    // --- Sensor + Encoder + Avoidance (every 100ms) ---
    if (now - lastScan >= SCAN_INTERVAL_MS) {
        lastScan = now;

        sensors.updateHeading();

        #if !DEBUG_MODE
            encoder.update(sensors.headingRad);
        #endif

        pathfinder.updateRobotWorld(encoder.posX, encoder.posY);

        if (pathfinder.isBacktracking()) {
            pathfinder.updateBacktrack(encoder.posX, encoder.posY);
        }

        bool avoiding = avoidance.update(sensors, motors);

        if (!avoiding) {
            updateMap();
            navigateWithPath();
        }
    }

    // --- Pathfinding Recalculation (every 500ms) ---
    if (now - lastPath >= PATH_UPDATE_MS) {
        lastPath = now;
        pathfinder.findPath();
    }
}

// ============================================
// TELEMETRY POST
// ============================================
void postTelemetry() {
    HTTPClient http;
    String url = "http://" + String(SERVER_HOST) + ":" + String(SERVER_PORT) + "/api/robot/report";

    http.begin(wifiClient, url);
    http.setTimeout(500);
    http.addHeader("Content-Type", "application/json");

    JsonDocument doc;
    doc["front"]       = sensors.distFront;
    doc["left"]        = sensors.distLeft;
    doc["right"]       = sensors.distRight;
    doc["heading"]     = sensors.heading;
    doc["pos_x"]       = encoder.posX;
    doc["pos_y"]       = encoder.posY;
    doc["distance"]    = encoder.totalDistCm;
    doc["enc_ticks"]   = encoder.getCount();
    doc["battery"]     = sensors.getBatteryVoltage();
    doc["state"]       = (int)avoidance.state;
    doc["path_length"] = pathfinder.pathLength;
    doc["auto"]        = autonomousMode;
    doc["grid_x"]      = pathfinder.robotPos.x;
    doc["grid_y"]      = pathfinder.robotPos.y;
    doc["target_x"]    = pathfinder.targetGrid.x;
    doc["target_y"]    = pathfinder.targetGrid.y;
    doc["target_wx"]   = pathfinder.targetWorldX;
    doc["target_wy"]   = pathfinder.targetWorldY;
    doc["has_target"]  = pathfinder.hasTarget;
    doc["target_reached"] = pathfinder.targetReached;
    doc["backtracking"]   = pathfinder.isBacktracking();
    doc["crumbs"]         = pathfinder.crumbCount;
    doc["debug_mode"]     = (bool)DEBUG_MODE;
    doc["wifi_rssi"]      = WiFi.RSSI();
    doc["free_heap"]      = ESP.getFreeHeap();
    doc["uptime"]         = millis() / 1000;

    String payload;
    serializeJson(doc, payload);
    http.POST(payload);
    http.end();
}

// ============================================
// FETCH COMMAND
// ============================================
void fetchCommand() {
    HTTPClient http;
    String url = "http://" + String(SERVER_HOST) + ":" + String(SERVER_PORT) + "/api/robot/command";

    http.begin(wifiClient, url);
    http.setTimeout(500);

    int code = http.GET();
    if (code == 200) {
        String body = http.getString();
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, body);

        if (!err) {
            const char* cmd = doc["cmd"] | "none";

            if (strcmp(cmd, "forward") == 0) {
                autonomousMode = false;
                motors.forward(SPEED_MEDIUM);
                Debug::log("[CMD] Manual: forward");
            } else if (strcmp(cmd, "back") == 0) {
                autonomousMode = false;
                motors.backward(SPEED_MEDIUM);
                Debug::log("[CMD] Manual: back");
            } else if (strcmp(cmd, "left") == 0) {
                autonomousMode = false;
                motors.turnLeft(SPEED_MEDIUM);
                Debug::log("[CMD] Manual: left");
            } else if (strcmp(cmd, "right") == 0) {
                autonomousMode = false;
                motors.turnRight(SPEED_MEDIUM);
                Debug::log("[CMD] Manual: right");
            } else if (strcmp(cmd, "stop") == 0) {
                autonomousMode = false;
                motors.stop();
                Debug::log("[CMD] Manual: stop");
            } else if (strcmp(cmd, "auto") == 0) {
                autonomousMode = true;
                Debug::log("[CMD] Autonomous mode");
            } else if (strcmp(cmd, "set_target") == 0) {
                float tx = doc["x"] | 0.0f;
                float ty = doc["y"] | 0.0f;
                pathfinder.setTargetWorld(tx, ty);
                autonomousMode = true;
                Debug::logf("[CMD] Target: X=%.0f Y=%.0f cm", tx, ty);
            } else if (strcmp(cmd, "backtrack") == 0) {
                pathfinder.startBacktrack();
                autonomousMode = true;
                Debug::log("[CMD] Backtrack started");
            } else if (strcmp(cmd, "reset") == 0) {
                encoder.resetPosition();
                pathfinder.begin();
                autonomousMode = true;
                Debug::log("[CMD] Position reset");
            // --- Hardware test commands ---
            } else if (strncmp(cmd, "test_", 5) == 0) {
                runTest(cmd);
            }
        }
    }
    http.end();
}

// ============================================
// HARDWARE TEST DISPATCHER
// ============================================
void runTest(const char* test) {
    autonomousMode = false;
    motors.stop();

    if (strcmp(test, "test_all") == 0) {
        testAll();
    } else if (strcmp(test, "test_i2c") == 0) {
        testI2C();
    } else if (strcmp(test, "test_ultrasonic") == 0) {
        testUltrasonic();
    } else if (strcmp(test, "test_servo") == 0) {
        testServo();
    } else if (strcmp(test, "test_mpu") == 0) {
        testMPU6050();
    } else if (strcmp(test, "test_motors") == 0) {
        testMotors();
    } else if (strcmp(test, "test_encoder") == 0) {
        testEncoder();
    } else if (strcmp(test, "test_battery") == 0) {
        testBattery();
    } else {
        Debug::logf("[TEST] Unknown test: %s", test);
    }

    debugFlush();
}

// ============================================
// NAVIGATION
// ============================================
void navigateWithPath() {
    if (pathfinder.targetReached) {
        motors.stop();
        return;
    }

    if (pathfinder.pathLength < 2) {
        if (pathfinder.hasTarget) {
            motors.forward(SPEED_SLOW);
        } else {
            motors.stop();
        }
        return;
    }

    int dir = pathfinder.getNextDirection(sensors.headingRad);

    switch (dir) {
        case 0:
            motors.forward(sensors.distFront > OBSTACLE_SLOW ? SPEED_MEDIUM : SPEED_SLOW);
            break;
        case -1:
            motors.curveLeft(SPEED_MEDIUM);
            break;
        case 1:
            motors.curveRight(SPEED_MEDIUM);
            break;
        case -2:
            motors.stop();
            break;
    }
}

// ============================================
// MAP UPDATE
// ============================================
void updateMap() {
    if (sensors.distFront < 300)
        pathfinder.markObstacle(sensors.distFront, sensors.headingRad);
    if (sensors.distLeft < 300)
        pathfinder.markObstacle(sensors.distLeft, sensors.headingRad - PI / 2);
    if (sensors.distRight < 300)
        pathfinder.markObstacle(sensors.distRight, sensors.headingRad + PI / 2);
}

// ============================================
// WIFI SETUP
// ============================================
void setupWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    Serial.print("Connecting to hotspot");
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConnected: " + WiFi.localIP().toString());
        Serial.printf("Signal: %d dBm\n", WiFi.RSSI());
        Debug::logf("WiFi connected: %s RSSI=%d", WiFi.localIP().toString().c_str(), WiFi.RSSI());
    } else {
        Serial.println("\nHotspot connection failed - running offline");
        Debug::log("WiFi FAILED");
    }
}

// ============================================
// DEBUG: Flush logs to server
// ============================================
void debugFlush() {
    if (Debug::logCount == 0) return;

    HTTPClient http;
    String url = "http://" + String(SERVER_HOST) + ":" + String(SERVER_PORT) + "/api/robot/debug";
    http.begin(wifiClient, url);
    http.setTimeout(300);
    http.addHeader("Content-Type", "application/json");

    JsonDocument doc;
    JsonArray arr = doc["logs"].to<JsonArray>();

    int start = (Debug::logHead - Debug::logCount + Debug::MAX_LOGS) % Debug::MAX_LOGS;
    for (int i = 0; i < Debug::logCount; i++) {
        int idx = (start + i) % Debug::MAX_LOGS;
        arr.add(Debug::logs[idx]);
    }

    String payload;
    serializeJson(doc, payload);
    http.POST(payload);
    http.end();
    Debug::logCount = 0;
}

// ============================================
// HARDWARE TESTS
// ============================================
void testI2C() {
    Debug::log("[TEST] I2C bus scan...");
    int found = 0;
    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            found++;
            if (addr == MPU6050_ADDR) Debug::logf("[I2C] 0x%02X = MPU6050 OK", addr);
            else                      Debug::logf("[I2C] 0x%02X = device found", addr);
        }
    }
    if (found == 0) Debug::log("[I2C] ERROR: No devices! Check SDA/SCL");
    else            Debug::logf("[I2C] Done: %d device(s)", found);
}

void testUltrasonic() {
    Debug::log("[TEST] Ultrasonic...");
    digitalWrite(US_TRIG, LOW);  delayMicroseconds(2);
    digitalWrite(US_TRIG, HIGH); delayMicroseconds(10);
    digitalWrite(US_TRIG, LOW);
    unsigned long dur = pulseIn(US_ECHO, HIGH, 30000);
    if (dur == 0) {
        Debug::log("[US] ERROR: No echo! Check TRIG->D4, ECHO->D7, VCC->5V");
    } else {
        Debug::logf("[US] %lu us = %.1f cm", dur, dur / 58.0);
        Debug::log("[US] OK");
    }
}

void testServo() {
    Debug::log("[TEST] Servo...");
    sensors.sweepServo.write(SERVO_CENTER); delay(500);
    Debug::log("[SERVO] CENTER");
    sensors.sweepServo.write(SERVO_LEFT);   delay(500);
    Debug::log("[SERVO] LEFT");
    sensors.sweepServo.write(SERVO_RIGHT);  delay(500);
    Debug::log("[SERVO] RIGHT");
    sensors.sweepServo.write(SERVO_CENTER); delay(300);
    Debug::log("[SERVO] Done");
}

void testMPU6050() {
    Debug::log("[TEST] MPU6050...");
    Wire.beginTransmission(MPU6050_ADDR);
    if (Wire.endTransmission() != 0) {
        Debug::logf("[MPU] ERROR: No response at 0x%02X", MPU6050_ADDR);
        return;
    }
    Wire.beginTransmission(MPU6050_ADDR); Wire.write(0x6B); Wire.write(0x00); Wire.endTransmission();
    delay(50);
    // WHO_AM_I
    Wire.beginTransmission(MPU6050_ADDR); Wire.write(0x75); Wire.endTransmission();
    Wire.requestFrom((int)MPU6050_ADDR, 1);
    if (Wire.available()) Debug::logf("[MPU] WHO_AM_I: 0x%02X (expect 0x68)", Wire.read());
    // Accel
    Wire.beginTransmission(MPU6050_ADDR); Wire.write(0x3B); Wire.endTransmission();
    Wire.requestFrom((int)MPU6050_ADDR, 6);
    if (Wire.available() >= 6) {
        int16_t ax = (Wire.read() << 8) | Wire.read();
        int16_t ay = (Wire.read() << 8) | Wire.read();
        int16_t az = (Wire.read() << 8) | Wire.read();
        Debug::logf("[MPU] Accel: X=%d Y=%d Z=%d", ax, ay, az);
        Debug::log(abs(az) > 10000 ? "[MPU] OK (gravity on Z)" : "[MPU] WARNING: weak Z");
    }
}

void testMotors() {
    Debug::log("[TEST] Motors (1 sec each)...");
    Debug::log("[MOTOR] Left FWD"); analogWrite(MOTOR_L_IN1, SPEED_SLOW); analogWrite(MOTOR_L_IN2, 0); analogWrite(MOTOR_R_IN1, 0); analogWrite(MOTOR_R_IN2, 0); delay(1000);
    Debug::log("[MOTOR] Left BWD"); analogWrite(MOTOR_L_IN1, 0); analogWrite(MOTOR_L_IN2, SPEED_SLOW); delay(1000); analogWrite(MOTOR_L_IN2, 0);
    Debug::log("[MOTOR] Right FWD"); analogWrite(MOTOR_R_IN1, SPEED_SLOW); delay(1000);
    Debug::log("[MOTOR] Right BWD"); analogWrite(MOTOR_R_IN1, 0); analogWrite(MOTOR_R_IN2, SPEED_SLOW); delay(1000); analogWrite(MOTOR_R_IN2, 0);
    Debug::log("[MOTOR] Both FWD"); analogWrite(MOTOR_L_IN1, SPEED_SLOW); analogWrite(MOTOR_R_IN1, SPEED_SLOW); delay(1000);
    analogWrite(MOTOR_L_IN1, 0); analogWrite(MOTOR_R_IN1, 0);
    Debug::log("[MOTOR] Done");
}

void testEncoder() {
    Debug::log("[TEST] Encoder...");
    #if DEBUG_MODE
        Debug::log("[ENC] Debug mode ON - encoder disabled (Serial uses RX)");
        Debug::log("[ENC] Set DEBUG_MODE 0 and re-upload to test encoder");
    #else
        long start = encoderCount;
        Debug::logf("[ENC] Ticks: %ld. Push robot for 3 sec...", start);
        delay(3000);
        long diff = encoderCount - start;
        Debug::logf("[ENC] Delta: %ld ticks (%.1f cm)", diff, diff * 1.021);
        Debug::log(diff == 0 ? "[ENC] ERROR: No ticks! Check RX wiring" : "[ENC] OK");
    #endif
}

void testBattery() {
    Debug::log("[TEST] Battery...");
    int raw = analogRead(A0);
    float v = (raw / 1023.0) * BATTERY_MAX_V;
    Debug::logf("[BATT] ADC=%d Voltage=%.2fV", raw, v);
    if (raw > 1000)     Debug::log("[BATT] WARNING: saturated, need divider?");
    else if (v < 4.0)   Debug::log("[BATT] WARNING: low battery!");
    else                 Debug::log("[BATT] OK");
}

void testAll() {
    Debug::log("========== FULL HARDWARE TEST ==========");
    testBattery();
    testI2C();
    testMPU6050();
    testUltrasonic();
    testServo();
    testMotors();
    testEncoder();
    Debug::log("========== TEST COMPLETE ==========");
}
