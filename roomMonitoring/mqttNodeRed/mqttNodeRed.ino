#include <WiFi.h>
#include <PubSubClient.h>
#include "DHTesp.h"

const int DHT_PIN = 18;  // DHT22 sensor pin

DHTesp dhtSensor;

const char* ssid = "change this";  // Wi-Fi SSID
const char* password = "change this";      // Wi-Fi password
const char* mqtt_server = "broker.hivemq.com";  // MQTT broker address
const int mqtt_port = 1883;  // MQTT port


WiFiClient espClient;
PubSubClient client(espClient);  // MQTT client object
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (50)
float temp = 0;
float hum = 0;
int value = 0;

void setup_wifi() {
  delay(10);
  // Connecting to WiFi
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(2, LOW);   // Turn the LED on
  } else {
    digitalWrite(2, HIGH);  // Turn the LED off
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("Connected");
      // Once connected, publish an announcement...
      client.publish("sensor/mqtt", "sensor");
      // ... and resubscribe
      client.subscribe("sensor/mqtt");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  pinMode(2, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  Serial.begin(115200);
  setup_wifi();  // Connect to Wi-Fi
  // Set the MQTT server for WebSocket connection (HiveMQ WebSocket port)
  client.setServer(mqtt_server, mqtt_port);  // WebSocket port for HiveMQ is 8000
  client.setCallback(callback);
  dhtSensor.setup(DHT_PIN, DHTesp::DHT22);  // Initialize DHT sensor
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();  // MQTT loop to handle incoming and outgoing messages

  unsigned long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;
    TempAndHumidity data = dhtSensor.getTempAndHumidity();  // Get data from DHT22

    String temp = String(data.temperature, 2);
    Serial.print("Temperature: ");
    Serial.println(temp);
    client.publish("sensor/temperature", temp.c_str());  // Publish temperature

    String hum = String(data.humidity, 1);
    Serial.print("Humidity: ");
    Serial.println(hum);
    client.publish("sensor/humidity", hum.c_str());  // Publish humidity
  }
}
