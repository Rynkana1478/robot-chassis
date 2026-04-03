// ============================================
// DRV8833 Motor Driver + Wheel Encoder Test
// Tests each motor channel and encoder counting
//
// Pin Assignment (matches robot project):
//   D5 (GPIO14) = Left  motors FWD (AIN1)
//   D6 (GPIO12) = Left  motors BWD (AIN2)
//   D0 (GPIO16) = Right motors FWD (BIN1)
//   D8 (GPIO15) = Right motors BWD (BIN2)
//   RX (GPIO3)  = Encoder signal
//
// DRV8833 wiring:
//   VCC = 6V battery
//   GND = common ground with ESP
//   SLP = 3.3V (always active)
//
// NOTE: Serial is disabled after encoder init (uses RX pin)
//       All output happens BEFORE encoder starts, OR
//       encoder test runs last with results shown by LED blinks
//
// Serial Monitor at 115200
// ============================================

#include <Arduino.h>

// --- Motor Pins ---
#define MOTOR_L_IN1  14  // D5
#define MOTOR_L_IN2  12  // D6
#define MOTOR_R_IN1  16  // D0
#define MOTOR_R_IN2  15  // D8

// --- Encoder ---
#define ENCODER_PIN  3   // GPIO3 (RX)

// --- Test Speeds ---
#define TEST_SPEED_LOW   350
#define TEST_SPEED_MED   550
#define TEST_SPEED_HIGH  750

// --- LED for encoder feedback (onboard) ---
#define LED_PIN 2  // D4 (onboard LED, active LOW)

// Encoder ISR
volatile long encoderCount = 0;
void IRAM_ATTR encoderISR() { encoderCount++; }

// ============================================
// MOTOR HELPERS
// ============================================
void stopAll() {
    analogWrite(MOTOR_L_IN1, 0);
    analogWrite(MOTOR_L_IN2, 0);
    analogWrite(MOTOR_R_IN1, 0);
    analogWrite(MOTOR_R_IN2, 0);
}

void brakeAll() {
    digitalWrite(MOTOR_L_IN1, HIGH);
    digitalWrite(MOTOR_L_IN2, HIGH);
    digitalWrite(MOTOR_R_IN1, HIGH);
    digitalWrite(MOTOR_R_IN2, HIGH);
}

void leftFwd(int speed)  { analogWrite(MOTOR_L_IN1, speed); analogWrite(MOTOR_L_IN2, 0); }
void leftBwd(int speed)  { analogWrite(MOTOR_L_IN1, 0);     analogWrite(MOTOR_L_IN2, speed); }
void rightFwd(int speed) { analogWrite(MOTOR_R_IN1, speed); analogWrite(MOTOR_R_IN2, 0); }
void rightBwd(int speed) { analogWrite(MOTOR_R_IN1, 0);     analogWrite(MOTOR_R_IN2, speed); }

// Blink LED n times (for post-Serial feedback)
void blinkLED(int n, int onMs = 200, int offMs = 200) {
    for (int i = 0; i < n; i++) {
        digitalWrite(LED_PIN, LOW);   // ON (active low)
        delay(onMs);
        digitalWrite(LED_PIN, HIGH);  // OFF
        delay(offMs);
    }
}

// ============================================
// TEST 1: MOTOR PIN CHECK (no movement)
// ============================================
void testPins() {
    Serial.println("=== Test 1: Motor Pin Check ===");
    Serial.println("  Setting each pin HIGH briefly to verify wiring");
    Serial.println("  (DRV8833 SLP must be connected to 3.3V)\n");

    const int pins[] = {MOTOR_L_IN1, MOTOR_L_IN2, MOTOR_R_IN1, MOTOR_R_IN2};
    const char* names[] = {"D5 (L-FWD)", "D6 (L-BWD)", "D0 (R-FWD)", "D8 (R-BWD)"};

    for (int i = 0; i < 4; i++) {
        Serial.printf("  %s -> HIGH... ", names[i]);
        digitalWrite(pins[i], HIGH);
        delay(200);
        digitalWrite(pins[i], LOW);
        Serial.println("OK");
        delay(100);
    }
    Serial.println("  All pins toggled.\n");
}

// ============================================
// TEST 2: INDIVIDUAL MOTORS
// ============================================
void testIndividual() {
    Serial.println("=== Test 2: Individual Motor Test ===");
    Serial.println("  Each motor runs 1.5 sec FWD then 1.5 sec BWD");
    Serial.println("  Watch which wheels move!\n");

    // Left FWD
    Serial.println("  [LEFT FWD] Left wheels should spin forward...");
    leftFwd(TEST_SPEED_MED);
    delay(1500);
    stopAll();
    delay(500);

    // Left BWD
    Serial.println("  [LEFT BWD] Left wheels should spin backward...");
    leftBwd(TEST_SPEED_MED);
    delay(1500);
    stopAll();
    delay(500);

    // Right FWD
    Serial.println("  [RIGHT FWD] Right wheels should spin forward...");
    rightFwd(TEST_SPEED_MED);
    delay(1500);
    stopAll();
    delay(500);

    // Right BWD
    Serial.println("  [RIGHT BWD] Right wheels should spin backward...");
    rightBwd(TEST_SPEED_MED);
    delay(1500);
    stopAll();
    delay(500);

    Serial.println("  Individual test done.\n");
    Serial.println("  CHECKLIST:");
    Serial.println("  [ ] Left wheels moved forward then backward?");
    Serial.println("  [ ] Right wheels moved forward then backward?");
    Serial.println("  [ ] No wheels spun the wrong direction?");
    Serial.println("  [ ] All 4 wheels moved (2 left + 2 right)?\n");
    Serial.println("  If LEFT/RIGHT are swapped: swap AIN1<->BIN1 and AIN2<->BIN2 wires");
    Serial.println("  If FWD/BWD are swapped: swap IN1<->IN2 wires for that channel\n");
}

// ============================================
// TEST 3: COMBINED MOVEMENTS
// ============================================
void testCombined() {
    Serial.println("=== Test 3: Combined Movements ===\n");

    Serial.println("  [FORWARD] Both sides forward 2 sec...");
    leftFwd(TEST_SPEED_MED);
    rightFwd(TEST_SPEED_MED);
    delay(2000);
    stopAll();
    delay(500);

    Serial.println("  [BACKWARD] Both sides backward 2 sec...");
    leftBwd(TEST_SPEED_MED);
    rightBwd(TEST_SPEED_MED);
    delay(2000);
    stopAll();
    delay(500);

    Serial.println("  [SPIN LEFT] Left bwd + Right fwd 1 sec...");
    leftBwd(TEST_SPEED_MED);
    rightFwd(TEST_SPEED_MED);
    delay(1000);
    stopAll();
    delay(500);

    Serial.println("  [SPIN RIGHT] Left fwd + Right bwd 1 sec...");
    leftFwd(TEST_SPEED_MED);
    rightBwd(TEST_SPEED_MED);
    delay(1000);
    stopAll();
    delay(500);

    Serial.println("  [BRAKE] Full speed then brake...");
    leftFwd(TEST_SPEED_HIGH);
    rightFwd(TEST_SPEED_HIGH);
    delay(1000);
    brakeAll();
    delay(500);
    stopAll();

    Serial.println("  Combined test done.\n");
    Serial.println("  CHECKLIST:");
    Serial.println("  [ ] Robot drove straight forward?");
    Serial.println("  [ ] Robot drove straight backward?");
    Serial.println("  [ ] Robot spun left (counterclockwise)?");
    Serial.println("  [ ] Robot spun right (clockwise)?");
    Serial.println("  [ ] Brake stopped quickly (not coast)?\n");
}

// ============================================
// TEST 4: SPEED RAMP
// ============================================
void testSpeedRamp() {
    Serial.println("=== Test 4: Speed Ramp ===");
    Serial.println("  Ramping from 0 to max and back\n");

    Serial.println("  Ramp UP...");
    for (int speed = 0; speed <= 1023; speed += 50) {
        leftFwd(speed);
        rightFwd(speed);
        Serial.printf("    PWM: %4d / 1023\n", speed);
        delay(300);
    }

    Serial.println("  Ramp DOWN...");
    for (int speed = 1023; speed >= 0; speed -= 50) {
        leftFwd(speed);
        rightFwd(speed);
        Serial.printf("    PWM: %4d / 1023\n", speed);
        delay(300);
    }

    stopAll();
    Serial.println("  Speed ramp done.\n");
    Serial.println("  Did the robot smoothly accelerate and decelerate?\n");
}

// ============================================
// TEST 5: ENCODER (Serial disabled!)
// LED blinks = result
// ============================================
void testEncoder() {
    // This is the LAST test because it kills Serial
    Serial.println("=== Test 5: Encoder ===");
    Serial.println("  Serial will be DISABLED after this (encoder uses RX pin)");
    Serial.println("  Results shown by onboard LED blinks:");
    Serial.println("    1 blink  = encoder connected but no ticks (push robot!)");
    Serial.println("    3 blinks = encoder counting OK");
    Serial.println("    5 fast   = encoder error / no response");
    Serial.println();
    Serial.println("  Phase A: Push robot by hand for 3 sec (LED solid = listening)");
    Serial.println("  Phase B: Motor drive + count for 2 sec");
    Serial.println();
    Serial.println("  Starting in 2 seconds...");
    delay(2000);

    // Disable Serial, enable encoder
    Serial.end();
    pinMode(ENCODER_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(ENCODER_PIN), encoderISR, RISING);

    // --- Phase A: Manual push ---
    // LED solid = listening
    digitalWrite(LED_PIN, LOW);  // ON
    encoderCount = 0;
    delay(3000);  // Wait for user to push
    digitalWrite(LED_PIN, HIGH); // OFF
    long manualTicks = encoderCount;

    delay(500);

    // --- Phase B: Motor drive + count ---
    encoderCount = 0;
    leftFwd(TEST_SPEED_MED);
    rightFwd(TEST_SPEED_MED);

    // LED blink while driving = counting
    for (int i = 0; i < 10; i++) {
        digitalWrite(LED_PIN, LOW);  delay(100);
        digitalWrite(LED_PIN, HIGH); delay(100);
    }

    stopAll();
    long motorTicks = encoderCount;

    delay(1000);

    // --- Report via LED ---
    // Phase A result
    if (manualTicks > 0) {
        blinkLED(3, 300, 300);  // 3 slow = manual push OK
    } else {
        blinkLED(1, 500, 500);  // 1 = no manual ticks
    }

    delay(1000);

    // Phase B result
    if (motorTicks > 5) {
        blinkLED(3, 300, 300);  // 3 slow = motor drive counted ticks
    } else if (motorTicks > 0) {
        blinkLED(2, 300, 300);  // 2 = some ticks (low)
    } else {
        blinkLED(5, 100, 100);  // 5 fast = no ticks at all
    }

    delay(2000);

    // Continuous: blink the tick count in groups of 10
    // (so user can roughly count)
    // e.g., 35 ticks = 3 slow blinks + pause + 5 fast blinks
    while (true) {
        int tens = motorTicks / 10;
        int ones = motorTicks % 10;

        // Show tens
        blinkLED(tens, 500, 300);
        delay(800);
        // Show ones
        blinkLED(ones, 150, 150);
        delay(3000);
    }
}

// ============================================
// SETUP
// ============================================
void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println();
    Serial.println("##################################################");
    Serial.println("#  DRV8833 Motor + Encoder Test                  #");
    Serial.println("#                                                 #");
    Serial.println("#  Pins: D5=L-FWD  D6=L-BWD                     #");
    Serial.println("#        D0=R-FWD  D8=R-BWD                     #");
    Serial.println("#        RX=Encoder                              #");
    Serial.println("#                                                 #");
    Serial.println("#  DRV8833: VCC=6V  GND=common  SLP=3.3V        #");
    Serial.println("##################################################\n");

    // Init motor pins
    pinMode(MOTOR_L_IN1, OUTPUT);
    pinMode(MOTOR_L_IN2, OUTPUT);
    pinMode(MOTOR_R_IN1, OUTPUT);
    pinMode(MOTOR_R_IN2, OUTPUT);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);  // LED off
    stopAll();

    // Run motor tests (Serial still active)
    testPins();
    testIndividual();
    testCombined();
    testSpeedRamp();

    Serial.println("##################################################");
    Serial.println("#  MOTOR TESTS COMPLETE                          #");
    Serial.println("#  Starting encoder test (Serial will stop)      #");
    Serial.println("##################################################\n");

    // Encoder test is last (kills Serial)
    testEncoder();
}

void loop() {
    // Never reached - testEncoder() loops forever with LED blinks
}
