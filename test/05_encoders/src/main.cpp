// ============================================
// Test 05: Dual Wheel Encoders
// Wire: Left=GPIO13, Right=GPIO14, VCC=3.3V, GND
// Tests: tick counting, direction, distance calc
// Push robot by hand to test (no motors needed)
// ============================================

#include <Arduino.h>

#define ENC_L_PIN 13
#define ENC_R_PIN 14

#define CM_PER_TICK 1.021  // 20 slots, 65mm wheel

volatile long encL = 0;
volatile long encR = 0;

void IRAM_ATTR isrL() { encL++; }
void IRAM_ATTR isrR() { encR++; }

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n##############################################");
    Serial.println("#  Test 05: Dual Wheel Encoders              #");
    Serial.println("#  Wire: Left=GPIO13, Right=GPIO14           #");
    Serial.println("#        VCC=3.3V, GND                       #");
    Serial.println("##############################################\n");

    pinMode(ENC_L_PIN, INPUT_PULLUP);
    pinMode(ENC_R_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(ENC_L_PIN), isrL, RISING);
    attachInterrupt(digitalPinToInterrupt(ENC_R_PIN), isrR, RISING);

    // === Test 1: Connection check ===
    Serial.println("=== Test 1: Connection Check ===");
    Serial.printf("  Left pin (GPIO%d):  %s\n", ENC_L_PIN,
        digitalRead(ENC_L_PIN) ? "HIGH (pullup OK)" : "LOW (sensor connected)");
    Serial.printf("  Right pin (GPIO%d): %s\n", ENC_R_PIN,
        digitalRead(ENC_R_PIN) ? "HIGH (pullup OK)" : "LOW (sensor connected)");
    Serial.println();

    // === Test 2: Manual push ===
    Serial.println("=== Test 2: Manual Push Test ===");
    Serial.println("  Push robot forward by hand for 5 seconds...\n");

    encL = 0;
    encR = 0;
    unsigned long start = millis();

    while (millis() - start < 5000) {
        static unsigned long lastPrint = 0;
        if (millis() - lastPrint >= 500) {
            lastPrint = millis();
            Serial.printf("  L: %4ld ticks (%5.1f cm)  |  R: %4ld ticks (%5.1f cm)\n",
                encL, encL * CM_PER_TICK, encR, encR * CM_PER_TICK);
        }
        delay(10);
    }

    Serial.printf("\n  Final: L=%ld R=%ld\n", encL, encR);

    if (encL == 0 && encR == 0) {
        Serial.println("  BOTH encoders: NO TICKS");
        Serial.println("  Check: VCC→3.3V, GND, OUT→GPIO13/14");
        Serial.println("  Check: encoder disc is on the shaft, sensor faces the slots");
    } else if (encL == 0) {
        Serial.println("  Left encoder: FAIL (no ticks)");
        Serial.println("  Right encoder: OK");
    } else if (encR == 0) {
        Serial.println("  Left encoder: OK");
        Serial.println("  Right encoder: FAIL (no ticks)");
    } else {
        Serial.println("  Both encoders: OK");
        float diff = abs(encL - encR) / (float)max(encL, encR) * 100;
        Serial.printf("  L/R difference: %.0f%% ", diff);
        Serial.println(diff < 20 ? "(normal)" : "(high - check mounting)");
    }

    Serial.println("\n##############################################");
    Serial.println("#  Continuous counting (push robot around)   #");
    Serial.println("##############################################\n");

    encL = 0;
    encR = 0;
}

void loop() {
    static unsigned long last = 0;
    if (millis() - last < 200) return;
    last = millis();

    float distL = encL * CM_PER_TICK;
    float distR = encR * CM_PER_TICK;
    float distAvg = (distL + distR) / 2.0;

    Serial.printf("L: %5ld (%5.1fcm) | R: %5ld (%5.1fcm) | Avg: %5.1fcm\n",
        encL, distL, encR, distR, distAvg);
}
