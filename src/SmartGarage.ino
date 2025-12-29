/*
 * ████████████████████████████████████████████████████████████
 * █                                                          █
 * █          SMART GARAGE CONTROL SYSTEM                    █
 * █          ESP32 + MQTT + Pushsafer + ThingSpeak          █
 * █                                                          █
 * ████████████████████████████████████████████████████████████
 * 
 * Features:
 * - Automatic door control with vehicle detection
 * - Push notifications via Pushsafer
 * - Cloud logging via ThingSpeak
 * - MQTT integration with Node-RED
 * - Fire & intrusion detection
 * - Real-time sensor monitoring
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>
#include <Bonezegei_DHT22.h>

#include "config.h"
#include "SensorModule.h"
#include "PushsaferNotifier.h"
#include "ThingSpeakLogger.h"

// ============================================
// GLOBAL OBJECTS
// ============================================

WiFiClient espClient;
PubSubClient mqttClient(espClient);

Servo servoDoor;
Servo servoExtinguisher;

Bonezegei_DHT22 dht(DHT_PIN);

PushsaferNotifier pushNotifier;
ThingSpeakLogger cloudLogger;

// ============================================
// STATE VARIABLES
// ============================================

DoorState doorState = DOOR_CLOSED;
AlarmState alarmState = ALARM_OFF;

bool vehicleDetectedOutside = false;
unsigned long vehicleDetectedTime = 0;

unsigned long lastMqttAttempt = 0;
unsigned long lastSensorRead = 0;
unsigned long lastThingSpeakUpload = 0;
unsigned long lastDistanceCheck = 0;

SensorData currentSensorData;

// ============================================
// FUNCTION PROTOTYPES
// ============================================

void connectWiFi();
void connectMQTT();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void checkVehicleDetection();
void checkFireDetection();
void checkIntrusionDetection();
void handleDoorControl();
void publishSensorData(const SensorData& data);

// ============================================
// SETUP
// ============================================

void setup() {
  Serial.begin(9600);
  delay(1000);
  
  printWelcomeBanner();
  
  // Initialize GPIO
  Serial.println("⚙️  Initializing hardware...");
  initializeGPIO();
  
  // Test LED
  testLED();
  
  // Initialize servos
  servoDoor.attach(SERVO_DOOR_PIN);
  servoExtinguisher.attach(SERVO_EXTINGUISHER_PIN);
  servoDoor.write(0);
  servoExtinguisher.write(0);
  Serial.println("   ✓ Servos initialized");
  
  // Initialize DHT22
  dht.begin();
  Serial.println("   ✓ DHT22 initialized");
  
  // Connect WiFi
  connectWiFi();
  
  // Initialize MQTT
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);
  Serial.println("   ✓ MQTT configured");
  
  // Initialize Pushsafer
  pushNotifier.begin();
  if (pushNotifier.isReady()) {
    pushNotifier.sendSystemOnline();
  }
  
  // Initialize ThingSpeak
  cloudLogger.begin(THINGSPEAK_API_KEY);
  
  Serial.println("\n████████████████████████████████████████████");
  Serial.println("█          SYSTEM READY!                  █");
  Serial.println("████████████████████████████████████████████\n");
  
  delay(1000);
}

// ============================================
// MAIN LOOP
// ============================================

void loop() {
  unsigned long now = millis();
  
  // Handle MQTT connection (non-blocking)
  if (!mqttClient.connected()) {
    if (now - lastMqttAttempt > MQTT_RETRY_INTERVAL) {
      lastMqttAttempt = now;
      connectMQTT();
    }
  } else {
    mqttClient.loop();
  }
  
  // Check vehicle detection
  if (now - lastDistanceCheck >= DISTANCE_CHECK_INTERVAL) {
    lastDistanceCheck = now;
    checkVehicleDetection();
  }
  
  // Read sensors periodically
  if (now - lastSensorRead >= SENSOR_READ_INTERVAL) {
    lastSensorRead = now;
    
    currentSensorData = readAllSensors(dht);
    printSensorData(currentSensorData);
    
    // Publish to MQTT
    publishSensorData(currentSensorData);
    
    // Check for critical conditions
    checkFireDetection();
    checkIntrusionDetection();
  }
  
  // Upload to ThingSpeak
  if (now - lastThingSpeakUpload >= THINGSPEAK_INTERVAL) {
    lastThingSpeakUpload = now;
    cloudLogger.uploadSensorData(currentSensorData);
  }
  
  // Handle door state
  handleDoorControl();
  
  delay(10);
}

// ============================================
// WIFI CONNECTION
// ============================================

void connectWiFi() {
  Serial.print("📡 Connecting to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(" connected!");
    Serial.print("   IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println(" FAILED!");
  }
}

// ============================================
// MQTT CONNECTION
// ============================================

void connectMQTT() {
  if (WiFi.status() != WL_CONNECTED) return;
  
  Serial.print("📡 Connecting to MQTT...");
  String clientId = "ESP32-Garage-" + String(random(0xffff), HEX);
  
  if (mqttClient.connect(clientId.c_str())) {
    Serial.println(" connected!");
    
    // Subscribe to control topics
    mqttClient.subscribe(TOPIC_DOOR_CMD);
    mqttClient.subscribe(TOPIC_ALARM_CMD);
    
    Serial.println("   ✓ Subscribed to control topics");
    
    // Publish online status
    mqttClient.publish(TOPIC_DOOR_STATUS, "ONLINE");
  } else {
    Serial.print(" failed, rc=");
    Serial.println(mqttClient.state());
  }
}

// ============================================
// MQTT CALLBACK
// ============================================

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  Serial.print("📩 MQTT [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(message);
  
  // Door control
  if (String(topic) == TOPIC_DOOR_CMD) {
    if (message == "OPEN") {
      doorState = DOOR_OPENING;
      Serial.println("→ Door command: OPEN");
    } else if (message == "CLOSE") {
      doorState = DOOR_CLOSING;
      Serial.println("→ Door command: CLOSE");
    }
  }
  
  // Alarm control
  if (String(topic) == TOPIC_ALARM_CMD) {
    if (message == "ON") {
      alarmState = ALARM_ON;
      mqttClient.publish(TOPIC_ALARM_STATUS, "ON");
      pushNotifier.sendAlarmActivated("Manual activation");
      Serial.println("→ Alarm: ON");
    } else if (message == "OFF") {
      alarmState = ALARM_OFF;
      mqttClient.publish(TOPIC_ALARM_STATUS, "OFF");
      pushNotifier.sendAlarmDeactivated("Manual");
      Serial.println("→ Alarm: OFF");
    }
  }
}

// ============================================
// VEHICLE DETECTION
// ============================================

void checkVehicleDetection() {
  float distance = readUltrasonic(ECHO_OUTSIDE_PIN, TRIG_OUTSIDE_PIN);
  
  if (distance < VEHICLE_DETECT_DISTANCE) {
    if (!vehicleDetectedOutside) {
      // New vehicle detected
      vehicleDetectedOutside = true;
      vehicleDetectedTime = millis();
      
      Serial.println("\n🚗 ========================================");
      Serial.print("   VEHICLE DETECTED: ");
      Serial.print(distance, 1);
      Serial.println(" cm");
      Serial.println("========================================");
      
      // Send push notification
      pushNotifier.sendVehicleDetected(distance);
      
      // Log to ThingSpeak
      cloudLogger.uploadEvent("VEHICLE_DETECTED", String(distance, 1));
      
      // Publish to MQTT
      mqttClient.publish(TOPIC_DOOR_STATUS, "VEHICLE_WAITING");
    }
    
    // Check timeout
    if (millis() - vehicleDetectedTime > WAIT_RESPONSE_TIME) {
      if (doorState == DOOR_CLOSED) {
        // No response - activate alert
        Serial.println("\n⚠️  NO RESPONSE - ACTIVATING ALERT");
        
        setLED(LED_OUTSIDE_PIN, true);
        activateBuzzer(BUZZER_PIN, 1000, 2000);
        
        delay(5000);
        setLED(LED_OUTSIDE_PIN, false);
        
        vehicleDetectedOutside = false;
      }
    }
  } else {
    vehicleDetectedOutside = false;
  }
}

// ============================================
// FIRE DETECTION
// ============================================

void checkFireDetection() {
  if (currentSensorData.temperatureDHT > TEMP_CRITICAL_THRESHOLD ||
      currentSensorData.smokeLevel > SMOKE_CRITICAL_THRESHOLD) {
    
    Serial.println("\n🔥 ========================================");
    Serial.println("   FIRE DETECTED!");
    Serial.println("========================================");
    
    // Send emergency notification
    pushNotifier.sendFireAlert(
      currentSensorData.temperatureDHT,
      currentSensorData.smokeLevel,
      currentSensorData.humidity
    );
    
    // Log to ThingSpeak
    cloudLogger.uploadEvent("FIRE_ALERT", "CRITICAL");
    
    // Publish to MQTT
    mqttClient.publish(TOPIC_ALARM_STATUS, "FIRE_DETECTED");
    
    // Activate extinguisher servo (example)
    servoExtinguisher.write(90);
    
    // Activate buzzer
    activateBuzzer(BUZZER_PIN, 2000, 5000);
  }
  else if (currentSensorData.temperatureDHT > TEMP_WARNING_THRESHOLD ||
           currentSensorData.smokeLevel > SMOKE_WARNING_THRESHOLD) {
    
    // Warning level
    if (currentSensorData.temperatureDHT > TEMP_WARNING_THRESHOLD) {
      pushNotifier.sendHighTemperature(currentSensorData.temperatureDHT);
    }
    
    if (currentSensorData.smokeLevel > SMOKE_WARNING_THRESHOLD) {
      pushNotifier.sendHighSmoke(currentSensorData.smokeLevel);
    }
  }
}

// ============================================
// INTRUSION DETECTION
// ============================================

void checkIntrusionDetection() {
  if (alarmState == ALARM_ON && 
      doorState == DOOR_CLOSED && 
      currentSensorData.pirMotion) {
    
    Serial.println("\n🚨 ========================================");
    Serial.println("   INTRUSION DETECTED!");
    Serial.println("========================================");
    
    alarmState = ALARM_TRIGGERED;
    
    // Send emergency notification
    pushNotifier.sendIntrusionAlert(true, true);
    
    // Log to ThingSpeak
    cloudLogger.uploadEvent("INTRUSION", "ALARM_TRIGGERED");
    
    // Publish to MQTT
    mqttClient.publish(TOPIC_ALARM_STATUS, "INTRUSION_DETECTED");
    
    // Activate alarm
    for (int i = 0; i < 10; i++) {
      setLED(LED_INSIDE_PIN, true);
      activateBuzzer(BUZZER_PIN, 2000, 200);
      delay(200);
      setLED(LED_INSIDE_PIN, false);
      delay(200);
    }
  }
}

// ============================================
// DOOR CONTROL
// ============================================

void handleDoorControl() {
  static DoorState lastState = DOOR_CLOSED;
  
  if (doorState != lastState) {
    switch (doorState) {
      case DOOR_OPENING:
        openDoor(servoDoor);
        doorState = DOOR_OPEN;
        mqttClient.publish(TOPIC_DOOR_STATUS, "OPENED");
        cloudLogger.uploadEvent("DOOR", "OPENED");
        pushNotifier.sendDoorOpened("User command");
        break;
        
      case DOOR_CLOSING:
        closeDoor(servoDoor);
        doorState = DOOR_CLOSED;
        mqttClient.publish(TOPIC_DOOR_STATUS, "CLOSED");
        cloudLogger.uploadEvent("DOOR", "CLOSED");
        pushNotifier.sendDoorClosed("User command");
        break;
        
      default:
        break;
    }
    
    lastState = doorState;
  }
}

// ============================================
// PUBLISH SENSOR DATA TO MQTT
// ============================================

void publishSensorData(const SensorData& data) {
  if (!mqttClient.connected()) return;
  
  char buffer[10];
  
  dtostrf(data.temperatureDHT, 4, 1, buffer);
  mqttClient.publish(TOPIC_TEMPERATURE, buffer);
  
  dtostrf(data.humidity, 4, 1, buffer);
  mqttClient.publish(TOPIC_HUMIDITY, buffer);
  
  itoa(data.smokeLevel, buffer, 10);
  mqttClient.publish(TOPIC_SMOKE, buffer);
  
  dtostrf(data.distanceOutside, 4, 1, buffer);
  mqttClient.publish(TOPIC_DISTANCE_OUT, buffer);
  
  dtostrf(data.distanceInside, 4, 1, buffer);
  mqttClient.publish(TOPIC_DISTANCE_IN, buffer);
  
  mqttClient.publish(TOPIC_PIR, data.pirMotion ? "DETECTED" : "CLEAR");
}

// ============================================
// HELPER FUNCTIONS
// ============================================

void initializeGPIO() {
  pinMode(TRIG_OUTSIDE_PIN, OUTPUT);
  pinMode(ECHO_OUTSIDE_PIN, INPUT);
  pinMode(TRIG_INSIDE_PIN, OUTPUT);
  pinMode(ECHO_INSIDE_PIN, INPUT);
  
  pinMode(LED_OUTSIDE_PIN, OUTPUT);
  pinMode(LED_INSIDE_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(PIR_PIN, INPUT);
  
  digitalWrite(LED_OUTSIDE_PIN, LOW);
  digitalWrite(LED_INSIDE_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);
  
  Serial.println("   ✓ GPIO initialized");
}

void testLED() {
  Serial.println("🔦 LED Self-Test...");
  blinkLED(LED_OUTSIDE_PIN, 3, 300);
  Serial.println("   ✓ LED test completed");
}

void printWelcomeBanner() {
  Serial.println("\n\n");
  Serial.println("████████████████████████████████████████████");
  Serial.println("█                                          █");
  Serial.println("█      SMART GARAGE CONTROL SYSTEM        █");
  Serial.println("█         ESP32 + IoT Integration         █");
  Serial.println("█                                          █");
  Serial.println("█  Features:                              █");
  Serial.println("█  • MQTT + Node-RED                      █");
  Serial.println("█  • Pushsafer Notifications              █");
  Serial.println("█  • ThingSpeak Cloud Logging             █");
  Serial.println("█  • Fire & Intrusion Detection           █");
  Serial.println("█                                          █");
  Serial.println("████████████████████████████████████████████");
  Serial.println();
}