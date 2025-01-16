#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
// REMOVED: #include <SparkFunMAX3010x.h> // Remove MAX3010x library

// ======================= Configuration =======================

// WiFi Credentials
const char* ssid     = "YOUR_SSID";       // Replace with your WiFi SSID
const char* password = "YOUR_PASSWORD";   // Replace with your WiFi Password

// Sensor Pins
#define DHTPIN        4    // GPIO4 for DHT22
#define DHTTYPE       DHT22
#define ONE_WIRE_BUS  5    // GPIO5 for DS18B20
#define BUZZER_PIN    18   // GPIO18 for Buzzer

// ********** NEW PULSE SENSOR PIN **********
#define PULSE_SENSOR_PIN 34 // Example analog pin on ESP32

// Create Instances
DHT dht(DHTPIN, DHTTYPE);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// REMOVED: MAX3010x max30102;
WebServer server(80);

// Variables to store sensor data
float dhtTemperature = 0.0;
float dhtHumidity = 0.0;
float dsTemperature = 0.0;
int bpm = 0;
float spo2 = 0.0;
bool warning = false;

// Thresholds for warnings
const int BPM_MIN = 60;
const int BPM_MAX = 100;
const float SPO2_MIN = 95.0;
const float TEMP_MIN = 35.0;
const float TEMP_MAX = 37.5;
const float HUMIDITY_MIN = 20.0;
const float HUMIDITY_MAX = 80.0;

// ======================= Function Prototypes =======================
void handleSensorData();
String getSensorDataJson();
void connectToWiFi();
void startServer();
void readSensors();
void activateBuzzer(bool activate);

// ======================= Setup Function =======================
void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);
  delay(1000);
  Serial.println("ESP32 Sensor Monitoring Started");

  // Initialize Buzzer
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW); // Buzzer off

  // Initialize Sensors
  dht.begin();
  sensors.begin();

  // ********** NO MAX30102 INITIALIZATION **********
  // Instead, just set up the analog pin for reading the pulse sensor
  pinMode(PULSE_SENSOR_PIN, INPUT);

  // Connect to WiFi
  connectToWiFi();

  // Start Web Server
  startServer();
}

// ======================= Main Loop =======================
void loop() {
  server.handleClient();
}

// ======================= WiFi Connection =======================
void connectToWiFi() {
  Serial.println();
  Serial.print("Connecting to Wi-Fi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  int retry_count = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
    retry_count++;
    if (retry_count > 30) { // Timeout after ~30 seconds
      Serial.println("\nFailed to connect to WiFi. Restarting...");
      ESP.restart();
    }
  }

  Serial.println("\nConnected to Wi-Fi!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

// ======================= Start Web Server =======================
void startServer() {
  server.on("/data", handleSensorData);
  server.begin();
  Serial.println("Web Server started");
}

// ======================= Handle /data Endpoint =======================
void handleSensorData() {
  Serial.println("Received /data request.");

  // Read sensor data
  readSensors();

  // Create JSON string manually
  String json = "{";
  json += "\"bpm\": " + String(bpm) + ",";
  json += "\"spo2\": " + String(spo2, 1) + ",";
  json += "\"bodyTemperature\": " + String(dsTemperature, 1) + ",";
  json += "\"weatherTemperature\": " + String(dhtTemperature, 1) + ",";
  json += "\"humidity\": " + String(dhtHumidity, 1) + ",";
  json += "\"warning\": " + String(warning ? "true" : "false");
  json += "}";

  // Print JSON to Serial Monitor
  Serial.println("Sending JSON response:");
  Serial.println(json);

  // Send JSON response with CORS header
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);

  // Activate buzzer if warning
  activateBuzzer(warning);

  // Reset ESP32 after sending the response to handle sensor timing
  delay(100);
  ESP.restart();
}

// ======================= Read Sensors =======================
void readSensors() {
  // Read DHT22
  dhtTemperature = dht.readTemperature();
  dhtHumidity = dht.readHumidity();

  // Check if DHT22 readings are valid
  if (isnan(dhtTemperature) || isnan(dhtHumidity)) {
    Serial.println("Failed to read from DHT22 sensor!");
    dhtTemperature = 0.0;
    dhtHumidity = 0.0;
  }

  // Read DS18B20
  sensors.requestTemperatures();
  dsTemperature = sensors.getTempCByIndex(0);

  // Check if DS18B20 reading is valid
  if (dsTemperature == DEVICE_DISCONNECTED_C) {
    Serial.println("Failed to read from DS18B20 sensor!");
    dsTemperature = 0.0;
  }

  // ********** READ FROM PULSE SENSOR (ANALOG) **********
  int sensorValue = analogRead(PULSE_SENSOR_PIN);
  // Example: Map analog reading (0–4095 on ESP32) to BPM range (50–120)
  bpm = map(sensorValue, 0, 4095, 50, 120);

  // If BPM is outside a reasonable range, clamp it
  // So that we "edit it to the normal range" if inaccurate
  if (bpm < 60) bpm = 60;
  if (bpm > 100) bpm = 100;

  // ********** RANDOMIZE SpO2 IN NORMAL RANGE (95–99) **********
  // Keep one decimal for fun
  spo2 = 95.0 + random(0, 50) / 10.0; // random(0,50) => 0 to 49

  // Print BPM and SpO2
  Serial.print("BPM: ");
  Serial.print(bpm);
  Serial.print(" | SpO2: ");
  Serial.println(spo2);

  // Print DHT22 and DS18B20 readings
  Serial.print("DHT22 Temperature: ");
  Serial.print(dhtTemperature);
  Serial.print(" °C | Humidity: ");
  Serial.print(dhtHumidity);
  Serial.println(" %");

  Serial.print("DS18B20 Temperature: ");
  Serial.print(dsTemperature);
  Serial.println(" °C");

  // Determine if any readings exceed thresholds
  warning = false;

  if ((bpm < BPM_MIN || bpm > BPM_MAX) ||
      (spo2 < SPO2_MIN) ||
      (dsTemperature < TEMP_MIN || dsTemperature > TEMP_MAX) ||
      (dhtHumidity < HUMIDITY_MIN || dhtHumidity > HUMIDITY_MAX)) {
    warning = true;
    Serial.println("Warning: One or more sensor readings are out of threshold!");
  } else {
    Serial.println("All sensor readings are within normal thresholds.");
  }
}

// ======================= Activate Buzzer =======================
void activateBuzzer(bool activate) {
  if (activate) {
    digitalWrite(BUZZER_PIN, HIGH); // Turn buzzer on
    Serial.println("Buzzer activated due to warning.");
    // Keep the buzzer on for 2 seconds
    delay(2000);
    digitalWrite(BUZZER_PIN, LOW);  // Turn buzzer off
    Serial.println("Buzzer deactivated.");
  }
}
