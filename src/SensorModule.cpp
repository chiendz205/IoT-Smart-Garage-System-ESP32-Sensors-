// SensorModule.cpp
#include "SensorModule.h"

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
  int raw = analogRead((gasPin));
  return map(raw, 0, 4095, 0, 1000);
}

// ============================================
// TEMPERATURE SENSOR (DS18B20 analog)
// ============================================
float readTemperatureSensor(int tempPin) {
  int rawValue = analogRead(tempPin);
  return (rawValue / 4095.0) * 100.0;
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
SensorData readAllSensors(DHTesp& dht) {
  SensorData data;
  data.timestamp = millis();

  // DHT22
TempAndHumidity values = dht.getTempAndHumidity();

data.temperatureDHT = values.temperature;
data.humidity    = values.humidity;


  // Other sensors
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
  Serial.print("Smoke Level:        "); Serial.print(data.smokeLevel); Serial.println(" ppm");
  Serial.print("Distance (Outside): "); Serial.print(data.distanceOutside, 1); Serial.println(" cm");
  Serial.print("Distance (Inside):  "); Serial.print(data.distanceInside, 1); Serial.println(" cm");
  Serial.print("PIR Motion:         "); Serial.println(data.pirMotion ? "DETECTED" : "None");
  Serial.println("========================================================\n");
}