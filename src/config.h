#ifndef CONFIG_H
#define CONFIG_H

// ============================================
// ESP8266 4WD Robot Chassis Configuration
// Motor Driver: DRV8833 | Battery: 6V
// Heading: MPU6050 gyro (no compass)
// ============================================

// --- Motor Pins (DRV8833) ---
#define MOTOR_L_IN1  14  // D5 - Left motors forward
#define MOTOR_L_IN2  12  // D6 - Left motors backward
#define MOTOR_R_IN1  16  // D0 - Right motors forward
#define MOTOR_R_IN2  15  // D8 - Right motors backward

// --- Ultrasonic Sensor (HC-SR04) ---
#define US_TRIG      2   // D4 - Trigger
#define US_ECHO      13  // D7 - Echo

// --- Servo (ultrasonic sweep) ---
#define SERVO_PIN    0   // D3

// --- I2C Bus (MPU6050) ---
#define I2C_SDA      4   // D2
#define I2C_SCL      5   // D1

// --- MPU6050 ---
#define MPU6050_ADDR 0x68

// --- Battery Monitor ---
#define BATTERY_PIN  A0
#define BATTERY_MAX_V 6.0

// --- Obstacle Thresholds (cm) ---
#define OBSTACLE_CLOSE   15
#define OBSTACLE_SLOW    30
#define OBSTACLE_CLEAR   50

// --- Motor Speeds (PWM 0-1023 on ESP8266) ---
#define SPEED_STOP   0
#define SPEED_SLOW   350
#define SPEED_MEDIUM 550
#define SPEED_FAST   750
#define SPEED_MAX    1023

// --- Servo Angles ---
#define SERVO_LEFT   160
#define SERVO_CENTER 90
#define SERVO_RIGHT  20

// --- Pathfinding Grid ---
#define GRID_SIZE    20
#define CELL_SIZE_CM 10

// --- Timing ---
#define SCAN_INTERVAL_MS    100
#define PATH_UPDATE_MS      500
#define SERVO_SWEEP_MS      150

// --- WiFi (phone hotspot) ---
#define WIFI_SSID     "Blackwise_2.4G"
#define WIFI_PASSWORD "0639041446"

// --- Flask Server (PC connected to same hotspot) ---
#define SERVER_HOST   "192.168.43.100"
#define SERVER_PORT   5000
#define REPORT_INTERVAL_MS  200

// --- Debug Mode ---
// 1 = Serial stays on (encoder disabled), debug logs + hardware tests
// 0 = Normal operation (encoder works, no Serial)
#define DEBUG_MODE  1

// --- Gyro Heading ---
// Gyro Z dead zone: ignore rotation below this (deg/s)
#define GYRO_DEADZONE  0.5

#endif // CONFIG_H
