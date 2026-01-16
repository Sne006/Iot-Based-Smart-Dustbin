/*
  Dry/Wet Waste Segregation
  Hardware: ESP32, HC-SR04, Soil Moisture (analog), Servo (ESP32Servo)
*/

#include <ESP32Servo.h>

// ---------------------------- Pin Definitions ----------------------------
#define TRIG_PIN 5
#define ECHO_PIN 18
#define MOISTURE_PIN 34
#define SERVO_PIN 13

// ---------------------------- Constants / Thresholds ----------------------------
#define DISTANCE_THRESHOLD 10        // cm - detect presence within this distance
#define MOISTURE_THRESHOLD 2000      // Lower = wet, Higher = dry (calibrate based on your sensor)
#define DRY_BIN_ANGLE 0
#define WET_BIN_ANGLE 135
#define NEUTRAL_ANGLE 90

// ---------------------------- Servo ----------------------------
Servo wasteServo;

// ---------------------------- Variables ----------------------------
long duration_us;
float distance_cm;
int moistureValue;
int binType = -1; // 0 = dry, 1 = wet

// ---------------------------- Sensors & Actuators ----------------------------
float getDistance() {
  // Trigger pulse
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Listen
  duration_us = pulseIn(ECHO_PIN, HIGH, 30000); // timeout 30ms -> ~5 meters
  if (duration_us == 0) {
    // no echo / out of range
    return -1.0;
  }
  distance_cm = (duration_us * 0.0343) / 2.0;
  return distance_cm;
}

int getMoistureLevel() {
  long sum = 0;
  const int samples = 5;
  for (int i = 0; i < samples; i++) {
    sum += analogRead(MOISTURE_PIN);
    delay(8);
  }
  return int(sum / samples);
}

void moveToWetBin() {
  Serial.println(">>> WET WASTE: moving to WET bin");
  wasteServo.write(WET_BIN_ANGLE);
  delay(1500);
  wasteServo.write(NEUTRAL_ANGLE);
}

void moveToDryBin() {
  Serial.println(">>> DRY WASTE: moving to DRY bin");
  wasteServo.write(DRY_BIN_ANGLE);
  delay(1500);
  wasteServo.write(NEUTRAL_ANGLE);
}

// ---------------------------- Setup ----------------------------
void setup() {
  Serial.begin(115200);
  delay(50);
  Serial.println("\n=== Smart Dustbin: Dry/Wet Segregation ===");

  // pins
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(MOISTURE_PIN, INPUT);

  // servo
  ESP32PWM::allocateTimer(0);
  wasteServo.setPeriodHertz(50);
  wasteServo.attach(SERVO_PIN, 500, 2400);
  wasteServo.write(NEUTRAL_ANGLE);
  delay(800);

  Serial.println("\n=== CALIBRATION MODE ===");
  Serial.println("Testing moisture sensor for 10 seconds...");
  Serial.println("Place sensor in DRY condition, then WET condition:");
  
  // Calibration loop
  for (int i = 0; i < 20; i++) {
    int testMoist = getMoistureLevel();
    Serial.print("Moisture reading: ");
    Serial.println(testMoist);
    delay(500);
  }
  
  Serial.println("=== Calibration complete. Adjust MOISTURE_THRESHOLD if needed ===");
  Serial.println("System ready. Place waste near ultrasonic sensor.\n");
}

// ---------------------------- Main Loop ----------------------------
void loop() {
  float dist = getDistance();
  if (dist > 0) {
    Serial.print("Distance: ");
    Serial.print(dist, 2);
    Serial.println(" cm");
  } else {
    Serial.println("Distance: out of range or no echo");
  }

  // Check presence
  if (dist > 0 && dist < DISTANCE_THRESHOLD) {
    Serial.println("\n*** WASTE DETECTED ***");
    delay(300); // small settle

    // read moisture
    int moist = getMoistureLevel();
    Serial.print("Moisture (avg ADC): ");
    Serial.print(moist);
    
    // Lower values = wetter
    if (moist < MOISTURE_THRESHOLD) {
      Serial.println(" --> WET waste detected");
      binType = 1; // wet
      moveToWetBin();
    } else {
      Serial.println(" --> DRY waste detected");
      binType = 0; // dry
      moveToDryBin();
    }

    Serial.println("Waiting before next detection...\n");
    delay(3000); // wait before next detection to avoid double-triggering
  }

  // brief idle
  delay(200);
}
