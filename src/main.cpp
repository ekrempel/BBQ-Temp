#include <Arduino.h>
#include <WiFi.h>

const char* ssid = "BBQ-Dev";
const char* password =  "BBQ-Dev-WIFI-01";

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

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("Connected to the WiFi network");
}

void loop() {
  Serial.println("Probe 1 Meat");
  double temp1 = measure(probe1meat, SteinhartA, SteinhartB, SteinhartC);
  Serial.println("Probe 2 Meat");
  double temp2 = measure(probe2meat, SteinhartA, SteinhartB, SteinhartC);
  Serial.println("Probe 2 Oven");
  double temp3 = measure(probe2oven, SteinhartA, SteinhartB, SteinhartC);
  delay(1000);                 

}