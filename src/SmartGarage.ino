/*
   SMART GARAGE CONTROL SYSTEM
   ESP32 + MQTT + Pushsafer + ThingSpeak
*/

#include <WiFi.h>
#include <PubSubClient.h>
#include "config.h"
#include "SensorModule.h"
#include "PushsaferNotifier.h"
#include "ThingSpeakLogger.h"

// Global Objects
WiFiClient espClient;
PubSubClient mqttClient(espClient);
Servo servoDoor;
Servo servoExtinguisher;
DHTesp dht;
PushsaferNotifier pushNotifier;
ThingSpeakLogger cloudLogger;

// State Variables
DoorState doorState = DOOR_CLOSED;
bool vehicleDetectedOutside = false;
unsigned long vehicleDetectedTime = 0;
unsigned long lastMqttAttempt = 0;
unsigned long lastSensorRead = 0;
unsigned long lastThingSpeakUpload = 0;
unsigned long lastDistanceCheck = 0;
SensorData currentSensorData;
AlarmState alarmState = ALARM_OFF;

// Function Prototypes
void connectWiFi();
void connectMQTT();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void checkVehicleDetection();
void checkFireDetection();
void checkIntrusionDetection();
void handleDoorControl();
void publishSensorData(const SensorData& data);

void setup() {
  Serial.begin(9600);
  delay(1000);

  printWelcomeBanner();

  // Initialize hardware
  Serial.println("⚙️ Initializing hardware...");
  initializeGPIO();

  servoDoor.attach(SERVO_DOOR_PIN);
  servoExtinguisher.attach(SERVO_EXTINGUISHER_PIN);
  servoDoor.write(0);
  servoExtinguisher.write(0);
  Serial.println("  ✓Servos initialized");

  dht.setup(DHT_PIN, DHTesp::DHT22);
  Serial.println("  ✓DHT22 initialized");

  // Connect WiFi
  connectWiFi();

  // Initialize MQTT
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);
  Serial.println("  ✓MQTT configured");

  // Initialize Pushsafer
  pushNotifier.begin();
  if (pushNotifier.isReady()) {
    pushNotifier.sendSystemOnline();
  }

  // Initialize ThingSpeak
  cloudLogger.begin(THINGSPEAK_API_KEY);

  Serial.println("\n========================================");
  Serial.println("SYSTEM READY!");
  Serial.println("========================================\n");

  delay(1000);
}

void loop() {
  unsigned long now = millis();

  // Handle alarm blinking
  handleAlarm();

  // MQTT connection
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

    publishSensorData(currentSensorData);

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

// WiFi Connection
void connectWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(" connected!");
    Serial.print("  IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println(" FAILED!");
  }
}

// MQTT Connection
void connectMQTT() {
  if (WiFi.status() != WL_CONNECTED) return;

  Serial.print("Connecting to MQTT...");
  String clientId = "ESP32-Garage-" + String(random(0xffff), HEX);

  if (mqttClient.connect(clientId.c_str())) {
    Serial.println(" connected!");

    mqttClient.subscribe(TOPIC_DOOR_CMD);
    mqttClient.subscribe(TOPIC_ALARM_CMD);

    Serial.println("  Subscribed to control topics");

    mqttClient.publish(TOPIC_DOOR_STATUS, "CLOSED");
  } else {
    Serial.print(" failed, rc=");
    Serial.println(mqttClient.state());
  }
}

// MQTT Callback
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.print("MQTT [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(message);

  // Door control
  if (String(topic) == TOPIC_DOOR_CMD) {
    if (message == "OPEN") {
      doorState = DOOR_OPENING;
      Serial.println("-> Door command: OPEN");
    } else if (message == "CLOSE") {
      doorState = DOOR_CLOSING;
      Serial.println("-> Door command: CLOSE");
    }
  }

  // Alarm control
  if (String(topic) == TOPIC_ALARM_CMD) {
    if (message == "ON") {
      alarmState = ALARM_ON;
      mqttClient.publish(TOPIC_ALARM_STATUS, "ON");
      pushNotifier.sendAlarmActivated("Manual activation");
      Serial.println("-> Alarm: ON");
    } else if (message == "OFF") {
      digitalWrite(LED_INSIDE_PIN, false);
      digitalWrite(LED_OUTSIDE_PIN, false);
      noTone(BUZZER_PIN);
      mqttClient.publish(TOPIC_ALARM_STATUS, "OFF");
      pushNotifier.sendAlarmDeactivated("Manual");
      Serial.println("-> Alarm: OFF");
      alarmState = ALARM_OFF;
    }
  }
}

// Alarm Manual Blinking
unsigned long lastBlink = 0;
bool blinkState = false;

void handleAlarm() {
  if (alarmState != ALARM_ON) return;

  unsigned long now = millis();

  if (now - lastBlink >= 200) {
    lastBlink = now;
    blinkState = !blinkState;

    digitalWrite(LED_INSIDE_PIN, blinkState);
    digitalWrite(LED_OUTSIDE_PIN, blinkState);

    if (blinkState)
      tone(BUZZER_PIN, 2000);
    else
      noTone(BUZZER_PIN);
  }
}

// Vehicle Detection
void checkVehicleDetection() {
  float distance = readUltrasonic(ECHO_OUTSIDE_PIN, TRIG_OUTSIDE_PIN);

  if (distance < VEHICLE_DETECT_DISTANCE) {
    if (!vehicleDetectedOutside) {
      vehicleDetectedOutside = true;
      vehicleDetectedTime = millis();

      Serial.println("\n========================================");
      Serial.print("🚗 VEHICLE DETECTED: ");
      Serial.print(distance, 1);
      Serial.println(" cm");
      Serial.println("========================================");

      pushNotifier.sendVehicleDetected(distance);
      cloudLogger.uploadEvent("VEHICLE_DETECTED", String(distance, 1));
      mqttClient.publish(TOPIC_VEHICLE_DETECTED, "true");
    }

    // Check timeout
    if (millis() - vehicleDetectedTime > WAIT_RESPONSE_TIME) {
      if (doorState == DOOR_CLOSED) {
        Serial.println("\n⚠️ NO RESPONSE - ACTIVATING ALERT");
        digitalWrite(LED_OUTSIDE_PIN, true);
        tone(BUZZER_PIN, 1000, 2000);

        delay(5000);
        digitalWrite(LED_OUTSIDE_PIN, false);
        vehicleDetectedOutside = false;
        mqttClient.publish(TOPIC_VEHICLE_DETECTED, "false");
      }
    }
  } else {
    if (vehicleDetectedOutside) {
      vehicleDetectedOutside = false;
      mqttClient.publish(TOPIC_VEHICLE_DETECTED, "false");
    }
  }
}

// Fire Detection
void checkFireDetection() {
  if (currentSensorData.temperatureDHT > TEMP_CRITICAL_THRESHOLD ||
      currentSensorData.smokeLevel > SMOKE_CRITICAL_THRESHOLD) {

    Serial.println("\n========================================");
    Serial.println("🔥 FIRE DETECTED!");
    Serial.println("========================================");

    alarmState = ALARM_FIRE;

    // LED alert
    for (int i = 0; i < 20; i++) {
      digitalWrite(LED_INSIDE_PIN, true);
      digitalWrite(LED_OUTSIDE_PIN, true);
      tone(BUZZER_PIN, 2000, 200);
      delay(100);
      digitalWrite(LED_INSIDE_PIN, false);
      digitalWrite(LED_OUTSIDE_PIN, false);
      delay(100);
    }

    // Send emergency notification
    mqttClient.publish(TOPIC_ALARM_STATUS, "FIRE_DETECTED");


    pushNotifier.sendFireAlert(
      currentSensorData.temperatureDHT,
      currentSensorData.smokeLevel,
      currentSensorData.humidity
    );

    // Activate fire extinguisher
    Serial.println("ACTIVATING FIRE EXTINGUISHER SERVO...");
    servoExtinguisher.write(90);
    delay(5000);
    servoExtinguisher.write(0);
    Serial.println("Fire extinguisher servo deactivated");
    
    // Send extinguisher notification
    pushNotifier.send("Fire Extinguisher Activated", 
                      "Servo chữa cháy đã được kích hoạt tự động", 
                      PRIORITY_EMERGENCY);

    cloudLogger.uploadEvent("FIRE_ALERT", "CRITICAL");
  
 
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
  else if (alarmState == ALARM_FIRE &&
           currentSensorData.temperatureDHT < TEMP_WARNING_THRESHOLD &&
           currentSensorData.smokeLevel < SMOKE_WARNING_THRESHOLD) {
    alarmState = ALARM_OFF;
    mqttClient.publish(TOPIC_ALARM_STATUS, "OFF");
  }
}

// Intrusion Detection
void checkIntrusionDetection() {
  if (doorState == DOOR_CLOSED &&
      currentSensorData.pirMotion) {

    Serial.println("\n========================================");
    Serial.println("🚨 INTRUSION DETECTED!");
    Serial.println("========================================");

    alarmState = ALARM_INTRUSION;

    // Activate alarm
    for (int i = 0; i < 10; i++) {
      digitalWrite(LED_INSIDE_PIN, true);
      digitalWrite(LED_OUTSIDE_PIN, true);
      tone(BUZZER_PIN, 2000, 200);
      delay(200);
      digitalWrite(LED_INSIDE_PIN, false);
      digitalWrite(LED_OUTSIDE_PIN, false);
      delay(200);
    }
    mqttClient.publish(TOPIC_ALARM_STATUS, "INTRUSION_DETECTED");
    pushNotifier.sendIntrusionAlert(true, true);
    cloudLogger.uploadEvent("INTRUSION", "CRITICAL");

  }
  else if (alarmState == ALARM_INTRUSION &&
           !currentSensorData.pirMotion) {
    alarmState = ALARM_OFF;
    mqttClient.publish(TOPIC_ALARM_STATUS, "OFF");
  }
}

// Door Control
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

// Publish Sensor Data to MQTT
void publishSensorData(const SensorData& data) {
  if (!mqttClient.connected()) return;

  mqttClient.publish(TOPIC_TEMPERATURE, String(data.temperatureDHT, 1).c_str());
  mqttClient.publish(TOPIC_HUMIDITY, String(data.humidity, 1).c_str());
  mqttClient.publish(TOPIC_SMOKE, String(data.smokeLevel).c_str());
  mqttClient.publish(TOPIC_DISTANCE_OUT, String(data.distanceOutside, 1).c_str());
  mqttClient.publish(TOPIC_DISTANCE_IN, String(data.distanceInside, 1).c_str());
  mqttClient.publish(TOPIC_PIR, data.pirMotion ? "DETECTED" : "CLEAR");
}

// Initialize GPIO
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

  Serial.println("  GPIO initialized");
}

// Print Welcome Banner
void printWelcomeBanner() {
  Serial.println("\n\n");
  Serial.println("========================================");
  Serial.println("SMART GARAGE CONTROL SYSTEM");
  Serial.println("ESP32 + IoT Integration");
  Serial.println("========================================");
  Serial.println();
}