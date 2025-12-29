// SensorModule.h
#ifndef SENSOR_MODULE_H
#define SENSOR_MODULE_H

#include <Arduino.h>
#include <ESP32Servo.h>
#include <Bonezegei_DHT22.h>
#include "config.h"

// ============================================
// SENSOR DATA STRUCTURE
// ============================================
struct SensorData {
  float temperatureDHT;
  float humidity;
  float temperatureDS;
  int smokeLevel;
  float distanceOutside;
  float distanceInside;
  bool pirMotion;
  unsigned long timestamp;
};

// ============================================
// ULTRASONIC SENSOR
// ============================================
float readUltrasonic(int echoPin, int trigPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  long duration = pulseIn(echoPin, HIGH, 30000);
  if (duration == 0) return MAX_DISTANCE;
  
  float distance = (duration * 0.034) / 2.0;
  return (distance > MAX_DISTANCE) ? MAX_DISTANCE : distance;
}

// ============================================
// PIR MOTION SENSOR
// ============================================
bool readPIR(int pirPin) {
  return digitalRead(pirPin) == HIGH;
}

// ============================================
// GAS/SMOKE SENSOR
// ============================================
int readGasSensor(int gasPin) {
  int rawValue = analogRead(gasPin);
  return map(rawValue, 0, 4095, 0, 1000);
}

// ============================================
// TEMPERATURE SENSOR (DS18B20 analog)
// ============================================
float readTemperatureSensor(int tempPin) {
  int rawValue = analogRead(tempPin);
  return (rawValue / 4095.0) * 100.0;
}

// ============================================
// LED CONTROL
// ============================================
void setLED(int pin, bool state) {
  digitalWrite(pin, state ? HIGH : LOW);
}

void blinkLED(int pin, int times, int delayMs) {
  for (int i = 0; i < times; i++) {
    digitalWrite(pin, HIGH);
    delay(delayMs);
    digitalWrite(pin, LOW);
    delay(delayMs);
  }
}

// ============================================
// BUZZER CONTROL
// ============================================
void activateBuzzer(int pin, int frequency, int durationMs) {
  tone(pin, frequency, durationMs);
}

void beep(int pin, int times) {
  for (int i = 0; i < times; i++) {
    tone(pin, 1000, 200);
    delay(300);
  }
}

// ============================================
// SERVO CONTROL
// ============================================
void moveDoorServo(Servo& servo, int angle) {
  servo.write(angle);
}

void openDoor(Servo& servo) {
  Serial.println("🚪 Opening door...");
  for (int pos = 0; pos <= 160; pos += 5) {
    servo.write(pos);
    delay(15);
  }
  Serial.println("✓ Door opened");
}

void closeDoor(Servo& servo) {
  Serial.println("🚪 Closing door...");
  for (int pos = 160; pos >= 0; pos -= 5) {
    servo.write(pos);
    delay(15);
  }
  Serial.println("✓ Door closed");
}

// ============================================
// READ ALL SENSORS
// ============================================
SensorData readAllSensors(Bonezegei_DHT22& dht) {
  SensorData data;
  data.timestamp = millis();
  
  // DHT22
  if (dht.getData()) {
    data.temperatureDHT = dht.getTemperature();
    data.humidity = dht.getHumidity();
  } else {
    data.temperatureDHT = -999;
    data.humidity = -999;
  }
  
  // Other sensors
  data.temperatureDS = readTemperatureSensor(TEMP_SENSOR_PIN);
  data.smokeLevel = readGasSensor(GAS_SENSOR_PIN);
  data.distanceOutside = readUltrasonic(ECHO_OUTSIDE_PIN, TRIG_OUTSIDE_PIN);
  data.distanceInside = readUltrasonic(ECHO_INSIDE_PIN, TRIG_INSIDE_PIN);
  data.pirMotion = readPIR(PIR_PIN);
  
  return data;
}

// ============================================
// PRINT SENSOR DATA
// ============================================
void printSensorData(const SensorData& data) {
  Serial.println("\n📊 ==================== SENSOR DATA ====================");
  Serial.print("Temperature (DHT):  "); Serial.print(data.temperatureDHT, 1); Serial.println(" °C");
  Serial.print("Humidity:           "); Serial.print(data.humidity, 1); Serial.println(" %");
  Serial.print("Temperature (DS):   "); Serial.print(data.temperatureDS, 1); Serial.println(" °C");
  Serial.print("Smoke Level:        "); Serial.print(data.smokeLevel); Serial.println(" ppm");
  Serial.print("Distance (Outside): "); Serial.print(data.distanceOutside, 1); Serial.println(" cm");
  Serial.print("Distance (Inside):  "); Serial.print(data.distanceInside, 1); Serial.println(" cm");
  Serial.print("PIR Motion:         "); Serial.println(data.pirMotion ? "DETECTED" : "None");
  Serial.println("========================================================\n");
}

#endif