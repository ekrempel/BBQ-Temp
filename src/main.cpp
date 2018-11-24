#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#define DEBUG true
#define BAUD 115200

const char* ssid = "BBQ-Dev";
const char* password =  "BBQ-Dev-WIFI-01";
const char* mqtt_server = "192.168.1.137";
// These constants won't change.  They're used to give names
// to the pins used:
const int analogInPin = GPIO_NUM_36;  // Analog input pin that the potentiometer is attached to -- Meat 1
//const int analogInPin = GPIO_NUM_39;  // Analog input pin that the potentiometer is attached to -- Meat 2
//const int analogInPin = GPIO_NUM_34;  // Analog input pin that the potentiometer is attached to -- Oven at Meat 

//these are really bad values! -27C at 25
//double SteinhartA=0.2284143823e-3;  // Steinhart-Hart A coefficient.
//double SteinhartB=2.373151289e-4; // Steinhart-Hart B coefficient.
//double SteinhartC=1.249333734e-7;  // Steinhart-Hart C coefficient.

const int probe1meat = GPIO_NUM_36;
const int probe2meat = GPIO_NUM_39;
const int probe2oven = GPIO_NUM_34;

double SteinhartA=0.3157857383e-3;  // Steinhart-Hart A coefficient.
double SteinhartB=2.238394535e-04; // Steinhart-Hart B coefficient.
double SteinhartC=-0.1725002939e-07;  // Steinhart-Hart C coefficient.

const double voltageSupply = 3.31;
const unsigned int bitResolution = pow(2, 12) - 1;
const unsigned long resistance = 566000;

WiFiClient espClient;
PubSubClient client(espClient);

double measure(int pinNumber, double steinhartA, double steinhartB, double steinhartC) {
  double adcReading = analogRead(analogInPin);
  double voltageReading = ((double) adcReading / (double) bitResolution) * voltageSupply;
  double resistanceThermistor = ((voltageSupply * (double) resistance) / voltageReading) - resistance;

  double lnResist;
  double temperature;

  lnResist = log(resistanceThermistor);
  temperature = steinhartA + (steinhartB * lnResist);
  temperature = temperature + (steinhartC * lnResist * lnResist * lnResist);
  temperature = (1.0 / temperature) - 273.15;

  Serial.print("adc: ");
  Serial.print(adcReading);
  Serial.print(" voltage: ");
  Serial.print(voltageReading);
  Serial.print(" resistance: ");
  Serial.print(resistanceThermistor);
  Serial.print(" temperature: ");
  Serial.print(temperature);
  Serial.println(" °C");

  return temperature;
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.println(topic);
}

void wifi_setup() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to ");
  Serial.print(ssid);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");

  Serial.print("SSID: ");
  Serial.print(ssid);
  Serial.print(" Status: connected");
  Serial.print(" IP: ");
  Serial.println(WiFi.localIP());
}

void mqtt_setup() {
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void mqtt_reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str(), "homeassistant", "erik1234")) {
      Serial.println("connected");

    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {

  if (DEBUG) {
    Serial.begin(BAUD);
  }

  wifi_setup();
  mqtt_setup();
}

void loop() {

  if (!client.connected()) {
    mqtt_reconnect();
  }
  client.loop();

  Serial.print("Probe 1 Meat - ");
  double temp1 = measure(probe1meat, SteinhartA, SteinhartB, SteinhartC);
  Serial.print("Probe 2 Meat - ");
  double temp2 = measure(probe2meat, SteinhartA, SteinhartB, SteinhartC);
  Serial.print("Probe 2 Oven - ");
  double temp3 = measure(probe2oven, SteinhartA, SteinhartB, SteinhartC);
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();

  JsonObject& probe1 = root.createNestedObject("probe1");
  JsonObject& probe2 = root.createNestedObject("probe2");

  probe1["meat"] = temp1;
  probe2["meat"] = temp2;
  probe2["oven"] = temp3;

  char jsonChar[100];
  
  root.printTo((char*)jsonChar, root.measureLength() + 1);
  client.publish("temperature", jsonChar);
  delay(1000);
}