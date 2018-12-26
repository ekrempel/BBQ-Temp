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
const int P1M = GPIO_NUM_34;
const int P1O = GPIO_NUM_35;

// Pins of Port 2, here we use a probe that only meassures the meat
const int P2M = GPIO_NUM_32;
// We dont read the second value, as we are not using a probe with two sensors here
//const int P2O = GPIO_NUM_25;

// // We need to provide steinhart values for the temperature probes.
// // Meat and oven probes stronly differ, as they have differnet ranges
// // We use meat probes that have slightly different values.
// // You can use this website to calibrate your probes
// // https://www.thinksrs.com/downloads/programs/therm%20calc/ntccalibrator/ntccalculator.html

//Steinhart Multy Probe - Meat
double Multi_Meat_Steinhart_A = 1.310531755e-3;  // Steinhart-Hart A coefficient.
double Multi_Meat_Steinhart_B = 0.9098587657e-4; // Steinhart-Hart B coefficient.
double Multi_Meat_Steinhart_C = 2.646335027e-7;  // Steinhart-Hart C coefficient.

// TODO MEASSURE OVEN RESISTORE AND FIX VALUES
//Steinhart Multy Probe - Oven
double Multi_Oven_Steinhart_A = 1.310531755e-3;  // Steinhart-Hart A coefficient.
double Multi_Oven_Steinhart_B = 0.9098587657e-4; // Steinhart-Hart B coefficient.
double Multi_Oven_Steinhart_C = 2.646335027e-7;  // Steinhart-Hart C coefficient.

//Steinhart Single Probe - Meat
double Single_Meat_Steinhart_A = 1.310531755e-3;  // Steinhart-Hart A coefficient.
double Single_Meat_Steinhart_B = 0.9098587657e-4; // Steinhart-Hart B coefficient.
double Single_Meat_Steinhart_C = 2.646335027e-7;  // Steinhart-Hart C coefficient.

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

  double Multi_Oven_Temp;
  double Multi_Meat_Temp;
  double Single_Meat_Temp;
  
  if(DEBUG)
  {
    Serial.print("MultiProbe - Oven");
    Multi_Oven_Temp = measure(P1O, Multi_Oven_Steinhart_A, Multi_Oven_Steinhart_B, Multi_Oven_Steinhart_C);
    Serial.print("MultiProbe - Meat");
    Multi_Meat_Temp = measure(P1M, Multi_Meat_Steinhart_A, Single_Meat_Steinhart_B, Multi_Meat_Steinhart_C);
    Serial.print("SinglProbe - Meat");
    Single_Meat_Temp = measure(P2M, Single_Meat_Steinhart_A, Single_Meat_Steinhart_B, Single_Meat_Steinhart_C);
  } else
  {
    Multi_Oven_Temp = measure(P1O, Multi_Oven_Steinhart_A, Multi_Oven_Steinhart_B, Multi_Oven_Steinhart_C);
    Multi_Meat_Temp = measure(P1M, Multi_Meat_Steinhart_A, Single_Meat_Steinhart_B, Multi_Meat_Steinhart_C);
    Single_Meat_Temp = measure(P2M, Single_Meat_Steinhart_A, Single_Meat_Steinhart_B, Single_Meat_Steinhart_C);
    Serial.print("| MultiProbe - Oven: ");
    Serial.print(Multi_Oven_Temp);
    Serial.print(" °C | MultiProbe - Meat: ");
    Serial.print(Multi_Meat_Temp);
    Serial.print(" °C | SingleProbe - Meat: ");
    Serial.print(Single_Meat_Temp);
    Serial.println(" °C |");
  }

  //create a buffer for the mqtt message
  StaticJsonBuffer<200> jsonBuffer;
  char jsonChar[100];

  //build a nested json message
  JsonObject& root = jsonBuffer.createObject();
  JsonObject& mp = root.createNestedObject("MP");
  JsonObject& sp = root.createNestedObject("SP");  
  mp["oven"] = Multi_Oven_Temp;
  mp["meat"] = Multi_Meat_Temp;
  sp["meat"] = Single_Meat_Temp;  

  root.printTo((char*)jsonChar, root.measureLength() + 1);
  //push the json message to mqtt topic temperature
  client.publish("temperature", jsonChar);
  
  //wait 5 seconds before we start the next meassurement
  delay(5000);
}