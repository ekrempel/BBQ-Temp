#include <Arduino.h>
#include <WiFi.h>

const char* ssid = "BBQ-Dev";
const char* password =  "BBQ-Dev-WIFI-01";

// These constants won't change.  They're used to give names
// to the pins used:
const int analogInPin = GPIO_NUM_36;  // Analog input pin that the potentiometer is attached to

//these are really bad values! -27C at 25
//double SteinhartA=0.2284143823e-3;  // Steinhart-Hart A coefficient.
//double SteinhartB=2.373151289e-4; // Steinhart-Hart B coefficient.
//double SteinhartC=1.249333734e-7;  // Steinhart-Hart C coefficient.

double SteinhartA=0.3157857383e-3;  // Steinhart-Hart A coefficient.
double SteinhartB=2.238394535e-04; // Steinhart-Hart B coefficient.
double SteinhartC=-0.1725002939e-07;  // Steinhart-Hart C coefficient.
double VoltageSupply = 3.24; // supply voltage of the thermistor-divider. Manually meter this for accuracy.
double VoltageReading; // current voltage in the middle point of the thermistor-divider.
unsigned long ResistanceFixed = 1031000; // fixed resistor between thermistor and ground, measured in ohms. Meter this for accuracy.
unsigned long ResistanceThermistor; // last calculated resistance of the thermistor, measured in ohms.
unsigned int ADCReading;
unsigned int BitResolution = pow(2, 12) - 1; // such as an 8bit, 10bit, or 12bit ADC. Most newer Arduino boards are 10bit.

double Temperature; // calculated temperature in Kelvin.

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }

  Serial.println("Connected to the WiFi network");
}

void loop() {
  // read the analog in value:
  ADCReading = analogRead(analogInPin);               

  // print the results to the serial monitor:
  Serial.print("sensor = " );                       
  Serial.println(ADCReading);      
  
  //Calculate the current voltage at Pin.
  //Voltage Divider Equation: VMidPoint = (RBottom / (RTop+RBottom)) * VSupply
  //...solving for: RTop = ((VSupply*Rbottom)/VMidPoint) - RBottom

  VoltageReading = ((double)ADCReading / (double)BitResolution) * VoltageSupply;
  ResistanceThermistor = ((VoltageSupply*(double)ResistanceFixed) / VoltageReading) - ResistanceFixed;

  Serial.print("voltage = " );                       
  Serial.println(VoltageReading); 
  
  Serial.print("R = " );                       
  Serial.println(ResistanceThermistor); 
  
  double LnResist;
  LnResist = log(ResistanceThermistor); // no reason to calculate this multiple times.
  Temperature = SteinhartA + (SteinhartB * LnResist);
  Temperature = Temperature + (SteinhartC * LnResist * LnResist * LnResist);
  Temperature = (1.0 / Temperature);
  
  Serial.print("Temp_K = " );                       
  Serial.println(Temperature); 
  
  Serial.print("Temp_C = " );                       
  Serial.println(Temperature - 273.15); 
  
  delay(1000);                 

}