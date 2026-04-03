// ============================================
// Test 06: Battery + BMS + Buck Converter
// Wire: Voltage divider (220K+33K) → GPIO1
// Tests: ADC reading, voltage calculation,
//        charge level, BMS protection
// ============================================

#include <Arduino.h>

#define BATT_PIN  1
#define BATT_MAX  8.4   // 2S fully charged
#define BATT_NOM  7.4   // 2S nominal
#define BATT_MIN  6.0   // 2S cutoff
// Divider: 220K + 33K → ratio = (220+33)/33 = 7.67

float readVoltage() {
    int raw = analogRead(BATT_PIN);
    float vadc = (raw / 4095.0) * 3.3;
    float vbatt = vadc * 7.67;  // divider ratio
    return vbatt;
}

int batteryPercent(float v) {
    if (v >= 8.2) return 100;
    if (v <= 6.0) return 0;
    return (int)((v - 6.0) / (8.2 - 6.0) * 100);
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n##############################################");
    Serial.println("#  Test 06: Battery + Power System           #");
    Serial.println("#  Wire: 220K+33K divider → GPIO1            #");
    Serial.println("#  Battery: 2S 18650 (6.0-8.4V)             #");
    Serial.println("##############################################\n");

    analogReadResolution(12);  // 12-bit ADC (0-4095)

    // === ADC raw test ===
    Serial.println("=== ADC Raw Reading ===");
    int raw = analogRead(BATT_PIN);
    float vadc = (raw / 4095.0) * 3.3;
    float vbatt = readVoltage();

    Serial.printf("  ADC raw: %d / 4095\n", raw);
    Serial.printf("  ADC voltage: %.3f V\n", vadc);
    Serial.printf("  Battery voltage: %.2f V\n", vbatt);
    Serial.printf("  Battery level: %d%%\n", batteryPercent(vbatt));

    // === Sanity checks ===
    Serial.println("\n=== Checks ===");

    if (raw == 0) {
        Serial.println("  ERROR: ADC reads 0");
        Serial.println("  - No voltage divider connected?");
        Serial.println("  - Battery not connected?");
        Serial.println("  - Wrong GPIO pin?");
    } else if (raw > 4000) {
        Serial.println("  WARNING: ADC near max (saturated)");
        Serial.println("  - Divider resistors wrong value?");
        Serial.println("  - Expected: 220K top + 33K bottom");
    } else if (vbatt < 5.0) {
        Serial.println("  WARNING: Voltage very low");
        Serial.println("  - Battery dead or not connected?");
        Serial.println("  - BMS may have cut off (over-discharge)");
    } else if (vbatt > 8.5) {
        Serial.println("  WARNING: Voltage too high");
        Serial.println("  - Divider ratio wrong?");
        Serial.println("  - Not actually 2S battery?");
    } else if (vbatt < BATT_MIN) {
        Serial.println("  BATTERY LOW - charge soon!");
    } else {
        Serial.println("  Battery: OK");
    }

    // === Voltage reference ===
    Serial.println("\n=== Expected Values ===");
    Serial.println("  Full charge: 8.4V (ADC ~1.10V, raw ~1360)");
    Serial.println("  Nominal:     7.4V (ADC ~0.96V, raw ~1195)");
    Serial.println("  Low:         6.5V (ADC ~0.85V, raw ~1050)");
    Serial.println("  Cutoff:      6.0V (ADC ~0.78V, raw ~970)");

    // === 5V buck test ===
    Serial.println("\n=== 5V Buck Converter ===");
    Serial.println("  Can't measure with ADC (no pin connected to 5V rail)");
    Serial.println("  Use a multimeter to verify 5V output from buck converter");
    Serial.println("  Check: ESP32 is powered, HC-SR04 gets 5V, servo gets 5V");

    Serial.println("\n##############################################");
    Serial.println("#  Continuous voltage monitor                #");
    Serial.println("##############################################\n");
}

void loop() {
    static unsigned long last = 0;
    if (millis() - last < 1000) return;
    last = millis();

    // Average 10 readings for stability
    float sum = 0;
    for (int i = 0; i < 10; i++) {
        sum += readVoltage();
        delay(5);
    }
    float v = sum / 10.0;
    int pct = batteryPercent(v);

    const char* status;
    if (v >= 7.8)      status = "FULL";
    else if (v >= 7.2) status = "GOOD";
    else if (v >= 6.5) status = "LOW";
    else               status = "CRITICAL";

    Serial.printf("Battery: %.2f V | %3d%% | %s\n", v, pct, status);
}
