// config.h
#ifndef CONFIG_H
#define CONFIG_H

// ============================================
// WIFI CONFIGURATION
// ============================================
#define WIFI_SSID     "Wokwi-GUEST"
#define WIFI_PASSWORD ""

// ============================================
// MQTT CONFIGURATION
// ============================================
#define MQTT_SERVER   "test.mosquitto.org"
#define MQTT_PORT     1883

// MQTT Topics
#define TOPIC_DOOR_CMD          "garage/door/cmd"
#define TOPIC_DOOR_STATUS       "garage/door/status"
#define TOPIC_TEMPERATURE       "garage/sensors/temperature"
#define TOPIC_HUMIDITY          "garage/sensors/humidity"
#define TOPIC_SMOKE             "garage/sensors/smoke"
#define TOPIC_DISTANCE_OUT      "garage/sensors/distance/outside"
#define TOPIC_DISTANCE_IN       "garage/sensors/distance/inside"
#define TOPIC_PIR               "garage/sensors/pir"
#define TOPIC_ALARM_STATUS      "garage/alarm/status"
#define TOPIC_ALARM_CMD         "garage/alarm/cmd"

// ============================================
// PUSHSAFER CONFIGURATION
// ============================================
#define PUSHSAFER_API_KEY       "YOUR_PUSHSAFER_KEY"
#define PUSHSAFER_API_URL       "https://www.pushsafer.com/api"

// ============================================
// THINGSPEAK CONFIGURATION
// ============================================
#define THINGSPEAK_API_KEY      "YOUR_THINGSPEAK_WRITE_KEY"
#define THINGSPEAK_CHANNEL_ID   0  // Your channel ID
#define THINGSPEAK_SERVER       "api.thingspeak.com"

// ============================================
// HARDWARE PIN DEFINITIONS
// ============================================

// Ultrasonic Sensors
#define ECHO_OUTSIDE_PIN        14
#define TRIG_OUTSIDE_PIN        12
#define ECHO_INSIDE_PIN         22
#define TRIG_INSIDE_PIN         23

// LEDs
#define LED_OUTSIDE_PIN         27
#define LED_INSIDE_PIN          4

// Buzzer
#define BUZZER_PIN              33

// PIR Motion Sensor
#define PIR_PIN                 32

// Servos
#define SERVO_DOOR_PIN          13
#define SERVO_EXTINGUISHER_PIN  15

// Sensors
#define DHT_PIN                 16
#define TEMP_SENSOR_PIN         35
#define GAS_SENSOR_PIN          34

// ============================================
// SYSTEM THRESHOLDS
// ============================================

// Distance thresholds
#define VEHICLE_DETECT_DISTANCE 100.0  // cm
#define MAX_DISTANCE            400.0  // cm

// Temperature thresholds
#define TEMP_WARNING_THRESHOLD  45.0   // °C
#define TEMP_CRITICAL_THRESHOLD 60.0   // °C

// Smoke/Gas thresholds
#define SMOKE_WARNING_THRESHOLD 600    // ppm
#define SMOKE_CRITICAL_THRESHOLD 800   // ppm

// Timing
#define WAIT_RESPONSE_TIME      10000  // 10 seconds
#define SENSOR_READ_INTERVAL    5000   // 5 seconds
#define THINGSPEAK_INTERVAL     20000  // 20 seconds (ThingSpeak limit: 15s)
#define MQTT_RETRY_INTERVAL     5000   // 5 seconds
#define DISTANCE_CHECK_INTERVAL 2000   // 2 seconds

// ============================================
// DOOR STATES
// ============================================
enum DoorState {
  DOOR_CLOSED,
  DOOR_OPENING,
  DOOR_OPEN,
  DOOR_CLOSING
};

// ============================================
// ALARM STATES
// ============================================
enum AlarmState {
  ALARM_OFF,
  ALARM_ON,
  ALARM_TRIGGERED
};

#endif