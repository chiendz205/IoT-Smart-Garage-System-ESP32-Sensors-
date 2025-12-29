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
// FUNCTION DECLARATIONS
// ============================================

// Ultrasonic Sensor
float readUltrasonic(int echoPin, int trigPin);

// PIR Motion Sensor
bool readPIR(int pirPin);

// Gas/Smoke Sensor
int readGasSensor(int gasPin);

// Temperature Sensor (DS18B20 analog)
float readTemperatureSensor(int tempPin);

// LED Control
void setLED(int pin, bool state);
void blinkLED(int pin, int times, int delayMs);

// Buzzer Control
void activateBuzzer(int pin, int frequency, int durationMs);
void beep(int pin, int times);

// Servo Control
void moveDoorServo(Servo& servo, int angle);
void openDoor(Servo& servo);
void closeDoor(Servo& servo);

// Read All Sensors
SensorData readAllSensors(Bonezegei_DHT22& dht);

// Print Sensor Data
void printSensorData(const SensorData& data);

#endif