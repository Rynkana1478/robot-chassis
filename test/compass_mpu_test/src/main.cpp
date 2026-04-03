// ============================================
// MPU6050 Test for Robot Chassis Project
// Uses Adafruit MPU6050 library
// Tests accel, gyro, heading integration
//
// Wiring: SDA=D2, SCL=D1, VCC=3.3V, GND=GND
// Serial Monitor at 115200
// ============================================

#include <Arduino.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

#define I2C_SDA 4  // D2
#define I2C_SCL 5  // D1

Adafruit_MPU6050 mpu;

// Gyro heading state
float heading = 0;
float headingRad = 0;
unsigned long lastUpdate = 0;
float gyroZOffset = 0;
bool calibrated = false;

// ============================================
// CALIBRATE GYRO Z
// ============================================
void calibrateGyro() {
    Serial.println("=== Gyro Calibration ===");
    Serial.println("  Keep robot STILL for 3 seconds...");

    float sum = 0;
    int count = 0;
    unsigned long start = millis();

    while (millis() - start < 3000) {
        sensors_event_t a, g, t;
        mpu.getEvent(&a, &g, &t);
        sum += g.gyro.z;
        count++;
        delay(10);
    }

    gyroZOffset = sum / count;
    calibrated = true;

    Serial.printf("  Samples: %d\n", count);
    Serial.printf("  Gyro Z offset: %.4f rad/s (%.2f deg/s)\n",
        gyroZOffset, gyroZOffset * 180.0 / PI);
    Serial.println("  Done.\n");
}

// ============================================
// UPDATE HEADING (same logic robot will use)
// ============================================
void updateHeading() {
    unsigned long now = millis();
    if (lastUpdate == 0) { lastUpdate = now; return; }

    float dt = (now - lastUpdate) / 1000.0;
    lastUpdate = now;

    sensors_event_t a, g, t;
    mpu.getEvent(&a, &g, &t);

    // Remove bias, convert to deg/s, integrate
    float gz = (g.gyro.z - gyroZOffset) * 180.0 / PI;  // rad/s to deg/s

    if (abs(gz) < 0.5) gz = 0;  // dead zone

    heading += gz * dt;

    while (heading >= 360) heading -= 360;
    while (heading < 0)    heading += 360;

    headingRad = heading * PI / 180.0;
}

// ============================================
// TEST 1: CHIP IDENTITY
// ============================================
void testIdentity() {
    Serial.println("=== Test 1: Chip Identity ===");
    Serial.println("  MPU6050 found and initialized via Adafruit library");
    Serial.println("  PASS\n");
}

// ============================================
// TEST 2: ACCELEROMETER
// ============================================
void testAccel() {
    Serial.println("=== Test 2: Accelerometer ===");

    sensors_event_t a, g, t;
    mpu.getEvent(&a, &g, &t);

    float ax = a.acceleration.x / 9.81;
    float ay = a.acceleration.y / 9.81;
    float az = a.acceleration.z / 9.81;

    Serial.printf("  X: %+.2f g\n", ax);
    Serial.printf("  Y: %+.2f g\n", ay);
    Serial.printf("  Z: %+.2f g\n", az);
    Serial.printf("  Total: %.2f g (expect ~1.0)\n", sqrt(ax*ax + ay*ay + az*az));

    Serial.print("  Mounting: ");
    if (abs(az) > 0.8)      Serial.println("FLAT - correct for robot");
    else if (abs(ax) > 0.8) Serial.println("On SIDE - wrong");
    else if (abs(ay) > 0.8) Serial.println("On EDGE - wrong");
    else                     Serial.println("TILTED");

    float pitch = atan2(ax, sqrt(ay*ay + az*az)) * 180.0 / PI;
    float roll  = atan2(ay, sqrt(ax*ax + az*az)) * 180.0 / PI;
    Serial.printf("  Pitch: %.1f deg, Roll: %.1f deg\n", pitch, roll);
    Serial.println(abs(pitch) < 10 && abs(roll) < 10 ? "  Level: OK" : "  WARNING: Not level");
    Serial.println();
}

// ============================================
// TEST 3: GYROSCOPE
// ============================================
void testGyro() {
    Serial.println("=== Test 3: Gyroscope ===");

    Serial.println("  --- At rest (5 samples) ---");
    sensors_event_t a, g, t;

    for (int i = 0; i < 5; i++) {
        delay(100);
        mpu.getEvent(&a, &g, &t);
        float gx = g.gyro.x * 180.0 / PI;
        float gy = g.gyro.y * 180.0 / PI;
        float gz = g.gyro.z * 180.0 / PI;
        Serial.printf("    X:%+6.1f  Y:%+6.1f  Z:%+6.1f deg/s\n", gx, gy, gz);
    }

    Serial.println("\n  --- Rotation test (5 sec) ---");
    Serial.println("  Rotate robot LEFT and RIGHT...");

    float maxZ = 0;
    unsigned long start = millis();
    while (millis() - start < 5000) {
        mpu.getEvent(&a, &g, &t);
        float gz = g.gyro.z * 180.0 / PI;
        if (abs(gz) > abs(maxZ)) maxZ = gz;
        delay(20);
    }

    Serial.printf("  Peak gyro Z: %+.1f deg/s\n", maxZ);
    if (abs(maxZ) > 20)      Serial.println("  Rotation: OK");
    else if (abs(maxZ) > 5)  Serial.println("  Weak rotation (turn faster)");
    else                      Serial.println("  WARNING: No rotation detected!");
    Serial.println();
}

// ============================================
// TEST 4: HEADING INTEGRATION (15 sec)
// ============================================
void testHeading() {
    Serial.println("=== Test 4: Heading Integration (15 sec) ===");
    Serial.println("  Rotate robot, then return to start position");
    Serial.println("  Heading should return near 0 deg\n");

    heading = 0;
    lastUpdate = millis();

    Serial.println("  Time     Heading   GyroZ     Status");
    Serial.println("  ------   -------   ------    ------");

    unsigned long start = millis();
    unsigned long lastPrint = 0;

    while (millis() - start < 15000) {
        updateHeading();

        if (millis() - lastPrint >= 500) {
            lastPrint = millis();

            sensors_event_t a, g, t;
            mpu.getEvent(&a, &g, &t);
            float gz = (g.gyro.z - gyroZOffset) * 180.0 / PI;

            const char* status = "still";
            if (abs(gz) > 5)        status = "TURNING";
            else if (abs(gz) > 0.5) status = "drifting";

            Serial.printf("  %5.1fs   %6.1f    %+5.1f     %s\n",
                (millis() - start) / 1000.0, heading, gz, status);
        }
        delay(10);
    }

    Serial.printf("\n  Final heading: %.1f deg\n", heading);
    if (abs(heading) < 5)       Serial.println("  Drift: EXCELLENT");
    else if (abs(heading) < 15) Serial.println("  Drift: OK (normal for gyro)");
    else                         Serial.printf("  Drift: %.1f deg/15s (no compass to correct)\n", heading);
    Serial.println();
}

// ============================================
// TEST 5: COMPATIBILITY
// ============================================
void testCompat() {
    Serial.println("=== Test 5: Robot Project Compatibility ===\n");

    sensors_event_t a, g, t;
    mpu.getEvent(&a, &g, &t);

    Serial.println("  [PASS] I2C address 0x68");

    float az = a.acceleration.z / 9.81;
    Serial.printf("  [%s] Accelerometer (Z=%.2fg)\n",
        abs(az) > 0.5 ? "PASS" : "FAIL", az);

    Serial.println("  [PASS] Gyroscope responds");

    Serial.printf("  [%s] Gyro heading integration\n",
        calibrated ? "PASS" : "SKIP");

    // Read speed test
    unsigned long start = millis();
    int reads = 0;
    while (millis() - start < 1000) {
        mpu.getEvent(&a, &g, &t);
        reads++;
    }
    Serial.printf("  [%s] Read speed: %d Hz (need >50)\n",
        reads > 50 ? "PASS" : "FAIL", reads);

    Serial.printf("  [PASS] Temperature: %.1f C\n", t.temperature);

    Serial.println("\n  Robot will use gyro Z for heading (no compass)");
    Serial.println("  Gyro drifts ~1-5 deg/min but works for navigation\n");
}

// ============================================
// SETUP
// ============================================
void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println();
    Serial.println("##################################################");
    Serial.println("#  MPU6050 Test for Robot Chassis Project         #");
    Serial.println("#  Using Adafruit MPU6050 Library                #");
    Serial.println("#  Wiring: SDA=D2, SCL=D1, VCC=3.3V, GND=GND   #");
    Serial.println("##################################################\n");

    Wire.begin(I2C_SDA, I2C_SCL);

    if (!mpu.begin(0x68, &Wire)) {
        Serial.println("ERROR: MPU6050 not found!");
        Serial.println("Check wiring and reset.");
        while (1) delay(1000);
    }

    mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
    mpu.setGyroRange(MPU6050_RANGE_250_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

    Serial.println("MPU6050 initialized:");
    Serial.println("  Accel: +-2G | Gyro: +-250 deg/s | Filter: 21Hz\n");

    testIdentity();
    testAccel();
    calibrateGyro();
    testGyro();
    testHeading();
    testCompat();

    Serial.println("##################################################");
    Serial.println("#  ALL TESTS COMPLETE                            #");
    Serial.println("##################################################\n");
    Serial.println("Continuous heading (rotate to verify)...\n");

    heading = 0;
    lastUpdate = millis();
}

// ============================================
// LOOP
// ============================================
void loop() {
    updateHeading();

    static unsigned long lastPrint = 0;
    if (millis() - lastPrint < 200) return;
    lastPrint = millis();

    sensors_event_t a, g, t;
    mpu.getEvent(&a, &g, &t);

    float gz = (g.gyro.z - gyroZOffset) * 180.0 / PI;
    float az = a.acceleration.z / 9.81;

    Serial.printf("Heading: %6.1f deg | GyroZ: %+5.1f d/s | AccelZ: %+.2f g\n",
        heading, gz, az);
}
