// ============================================
// Test 02: MPU6050 on ESP32-S3
// Wire: SDA=GPIO11, SCL=GPIO12, VCC=3.3V direct, GND
// Tests: I2C scan, accel, gyro, heading integration
// ============================================

#include <Arduino.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

#define I2C_SDA 11
#define I2C_SCL 12

Adafruit_MPU6050 mpu;
float heading = 0;
float gyroZOffset = 0;
unsigned long lastUpdate = 0;

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n##############################################");
    Serial.println("#  Test 02: MPU6050 on ESP32-S3              #");
    Serial.println("#  Wire: SDA=GPIO11, SCL=GPIO12              #");
    Serial.println("#        VCC=3.3V (direct!), GND             #");
    Serial.println("##############################################\n");

    Wire.begin(I2C_SDA, I2C_SCL);

    // I2C scan
    Serial.println("=== I2C Scan ===");
    int found = 0;
    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            Serial.printf("  0x%02X found%s\n", addr,
                addr == 0x68 ? " = MPU6050" : "");
            found++;
        }
    }
    if (found == 0) {
        Serial.println("  ERROR: No devices! Check wiring.");
        Serial.println("  SDA must be GPIO11, SCL must be GPIO12");
        while (1) delay(1000);
    }

    // Init MPU6050
    Serial.println("\n=== MPU6050 Init ===");
    if (!mpu.begin(0x68, &Wire)) {
        Serial.println("  ERROR: MPU6050 not responding!");
        while (1) delay(1000);
    }
    mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
    mpu.setGyroRange(MPU6050_RANGE_250_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
    Serial.println("  MPU6050 initialized OK\n");

    // Accel test
    Serial.println("=== Accelerometer ===");
    sensors_event_t a, g, t;
    mpu.getEvent(&a, &g, &t);
    float ax = a.acceleration.x / 9.81;
    float ay = a.acceleration.y / 9.81;
    float az = a.acceleration.z / 9.81;
    Serial.printf("  X: %+.2fg  Y: %+.2fg  Z: %+.2fg\n", ax, ay, az);
    Serial.printf("  Mounting: %s\n",
        abs(az) > 0.8 ? "FLAT (correct)" : "TILTED (check mounting)");
    Serial.printf("  Temp: %.1f C\n\n", t.temperature);

    // Calibrate gyro
    Serial.println("=== Gyro Calibration (keep still 2 sec) ===");
    float sum = 0;
    int count = 0;
    unsigned long start = millis();
    while (millis() - start < 2000) {
        mpu.getEvent(&a, &g, &t);
        sum += g.gyro.z;
        count++;
        delay(5);
    }
    gyroZOffset = sum / count;
    Serial.printf("  Offset: %.4f rad/s (%.2f deg/s)\n", gyroZOffset, gyroZOffset * 180.0 / PI);
    Serial.println("  Calibration done\n");

    // Gyro rotation test
    Serial.println("=== Rotation Test (5 sec) ===");
    Serial.println("  Rotate the board left/right...");
    float maxGz = 0;
    start = millis();
    while (millis() - start < 5000) {
        mpu.getEvent(&a, &g, &t);
        float gz = (g.gyro.z - gyroZOffset) * 180.0 / PI;
        if (abs(gz) > abs(maxGz)) maxGz = gz;
        delay(20);
    }
    Serial.printf("  Peak rotation: %+.1f deg/s\n", maxGz);
    Serial.println(abs(maxGz) > 10 ? "  Gyro responds: OK" : "  WARNING: no rotation detected");

    Serial.println("\n##############################################");
    Serial.println("#  Continuous heading (rotate to verify)     #");
    Serial.println("##############################################\n");

    heading = 0;
    lastUpdate = millis();
}

void loop() {
    // Update heading
    unsigned long now = millis();
    float dt = (now - lastUpdate) / 1000.0;
    lastUpdate = now;

    sensors_event_t a, g, t;
    mpu.getEvent(&a, &g, &t);
    float gz = (g.gyro.z - gyroZOffset) * 180.0 / PI;
    if (abs(gz) < 0.3) gz = 0;
    heading += gz * dt;
    while (heading >= 360) heading -= 360;
    while (heading < 0) heading += 360;

    static unsigned long lastPrint = 0;
    if (now - lastPrint >= 200) {
        lastPrint = now;
        Serial.printf("Heading: %6.1f deg | GyroZ: %+6.1f d/s | AccelZ: %+.2fg\n",
            heading, gz, a.acceleration.z / 9.81);
    }
    delay(10);
}
