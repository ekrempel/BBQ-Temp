#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "config.h"

#define DEBUG true
#define BAUD 115200

WiFiClient espClient;
PubSubClient client(espClient);

// // These values are used to calculate the temperature of the probes.
// Voltage provided by the board. Meassure with multimeter
const double voltageSupply = 3.25;
// fixed restistance values
const unsigned long resistance = 990000;
// resolution of the ADC on the board
const unsigned int bitResolution = pow(2, 12) - 1;

// // Define the Pins where we read the raw meassures from.
// // We have one temperature probe that meassures meat and oven temp
// // and an additional probe that meassures only the meat.
// // The current design of the platine would allow for two probes that
// // meassure meat and oven temp.

// Pins of Port 1, the first one meassures the meat, the second the oven temp
const int mp_oven = GPIO_NUM_35;
const int mp_meat = GPIO_NUM_34;

// Pins of Port 2, here we use a probe that only meassures the meat
const int sp_meat = GPIO_NUM_32;
// We dont read the second value, as we are not using a probe with two sensors here
//const int P2O = GPIO_NUM_25;

// // We need to provide steinhart values for the temperature probes.
// // Meat and oven probes stronly differ, as they have differnet ranges
// // We use meat probes that have slightly different values.
// // You can use this website to calibrate your probes
// // https://www.thinksrs.com/downloads/programs/therm%20calc/ntccalibrator/ntccalculator.html

//Steinhart Multy Probe - Meat
double mp_meat_Steinhart_A = 1.310531755e-3;  // Steinhart-Hart A coefficient.
double mp_meat_Steinhart_B = 0.9098587657e-4; // Steinhart-Hart B coefficient.
double mp_meat_Steinhart_C = 2.646335027e-7;  // Steinhart-Hart C coefficient.

// TODO MEASSURE OVEN RESISTORE AND FIX VALUES
//Steinhart Multy Probe - Oven
double mp_oven_Steinhart_A = 1.310531755e-3;  // Steinhart-Hart A coefficient.
double mp_oven_Steinhart_B = 0.9098587657e-4; // Steinhart-Hart B coefficient.
double mp_oven_Steinhart_C = 2.646335027e-7;  // Steinhart-Hart C coefficient.

//Steinhart Single Probe - Meat
double sp_meat_Steinhart_A = 1.310531755e-3;  // Steinhart-Hart A coefficient.
double sp_meat_Steinhart_B = 0.9098587657e-4; // Steinhart-Hart B coefficient.
double sp_meat_Steinhart_C = 2.646335027e-7;  // Steinhart-Hart C coefficient.

// function to calculate the temperate of a probe connected to a given Pin
double measure(int pinNumber, double steinhartA, double steinhartB, double steinhartC)
{
  // average of 25 reading to limit errors. Takes 50 ms for every sensor
  double adcReading = 0;
  for (int i = 0; i < 25; i++)
  {
    adcReading = adcReading + analogRead(pinNumber);
    delay(2); //let the adc settle for 2 ms
  }
  adcReading = adcReading / 25;

  // calculate the voltage of the ADC pin
  double voltageReading = ((double)adcReading / (double)bitResolution) * voltageSupply;
  // calculate the resistance value of the termistor
  double resistanceThermistor = ((voltageSupply * (double)resistance) / voltageReading) - resistance;

  double lnResist;
  double temperature;

  // calculate the temperature using the steinhart values provided
  lnResist = log(resistanceThermistor);
  temperature = steinhartA + (steinhartB * lnResist);
  temperature = temperature + (steinhartC * lnResist * lnResist * lnResist);
  temperature = (1.0 / temperature) - 273.15;

  // print additional debug messages
  if (DEBUG)
  {
    Serial.print("| Port: ");
    Serial.print(pinNumber);
    Serial.print(" | ADC: ");
    Serial.print(adcReading);
    Serial.print(" | Voltage: ");
    Serial.print(voltageReading);
    Serial.print(" | R: ");
    Serial.print(resistanceThermistor);
    Serial.print(" | Temp: ");
    Serial.print(temperature);
    Serial.println(" °C |");
  }
  return temperature;
}

void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.println(topic);
}

void wifi_setup()
{
  WiFi.begin(mySSID, myPASSWORD);
  Serial.print("Connecting to ");
  Serial.print(mySSID);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");

  Serial.print("SSID: ");
  Serial.print(mySSID);
  Serial.print(" Status: connected");
  Serial.print(" IP: ");
  Serial.println(WiFi.localIP());
}

void mqtt_setup()
{
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void mqtt_reconnect()
{
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");

    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str(), mqtt_server_user, mqtt_server_pw))
    {
      Serial.println("connected");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup()
{
  Serial.begin(BAUD);
  Serial.println("Starting setup:");
  wifi_setup();
  mqtt_setup();
}

void loop()
{

  //test if we have a mqtt connection and connect if not
  if (!client.connected()) {
    mqtt_reconnect();
  }
  //this will make the client read for mesages on mqtt
  client.loop();

  double mp_oven_temp;
  double mp_meat_temp;
  double sp_meat_temp;
  
  if(DEBUG)
  {
    Serial.print("MultiProbe - Oven");
    mp_oven_temp = measure(mp_oven, mp_oven_Steinhart_A, mp_oven_Steinhart_B, mp_oven_Steinhart_C);
    Serial.print("MultiProbe - Meat");
    mp_meat_temp = measure(mp_meat, mp_meat_Steinhart_A, sp_meat_Steinhart_B, mp_meat_Steinhart_C);
    Serial.print("SingleProbe - Meat");
    sp_meat_temp = measure(sp_meat, sp_meat_Steinhart_A, sp_meat_Steinhart_B, sp_meat_Steinhart_C);
  } else
  {
    mp_oven_temp = measure(mp_oven, mp_oven_Steinhart_A, mp_oven_Steinhart_B, mp_oven_Steinhart_C);
    mp_meat_temp = measure(mp_meat, mp_meat_Steinhart_A, sp_meat_Steinhart_B, mp_meat_Steinhart_C);
    sp_meat_temp = measure(sp_meat, sp_meat_Steinhart_A, sp_meat_Steinhart_B, sp_meat_Steinhart_C);
    Serial.print("| MultiProbe - Oven: ");
    Serial.print(mp_oven_temp);
    Serial.print(" °C | MultiProbe - Meat: ");
    Serial.print(mp_meat_temp);
    Serial.print(" °C | SingleProbe - Meat: ");
    Serial.print(sp_meat_temp);
    Serial.println(" °C |");
  }

  //create a buffer for the mqtt message
  StaticJsonBuffer<200> jsonBuffer;
  char jsonChar[100];

  //build a nested json message
  JsonObject& root = jsonBuffer.createObject();
  JsonObject& mp = root.createNestedObject("MP");
  JsonObject& sp = root.createNestedObject("SP");  
  mp["oven"] = mp_oven_temp;
  mp["meat"] = mp_meat_temp;
  sp["meat"] = sp_meat_temp;  

  root.printTo((char*)jsonChar, root.measureLength() + 1);
  //push the json message to mqtt topic temperature
  client.publish("temperature", jsonChar);
  
  //wait 5 seconds before we start the next meassurement
  delay(5000);
}