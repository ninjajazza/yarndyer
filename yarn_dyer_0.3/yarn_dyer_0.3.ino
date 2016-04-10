#include <OneWire.h>
#include <DallasTemperature.h>

// Data wire is plugged into pin 2 on the Arduino
#define ONE_WIRE_BUS 2

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

// set variables for run
unsigned long setTime, setTimeMins, currentTime;
float setTemp, currentTemp;

const int relayPin = 1;

void setup(void)
{
  // start serial port
  Serial.begin(9600);
  Serial.println("Dallas Temperature IC Control Library Demo");

  // Start up the Dallas temp library
  sensors.begin();

  // Set run values
  setTimeMins = 1; // in millis
  setTime = setTimeMins * 60 * 1000; // in millis
  setTemp = 22; // in C
  
  pinMode(relayPin, OUTPUT);
}

void loop(void)
{ 
   currentTime = millis();
   currentTemp = getTemp();   
   
   Serial.println("Temp: " + String(currentTemp) + ", Time:" + currentTime);

   if (currentTime > setTime) {
    Serial.println("Run complete");
    digitalWrite(relayPin, LOW);
   } else {
    Serial.println("Still running");
    if (currentTemp < setTemp) {
      digitalWrite(relayPin, HIGH);
      } else {
        digitalWrite(relayPin, LOW);
      }
   }
  
}

float getTemp(void) {
  sensors.requestTemperatures(); 
  float temp = sensors.getTempCByIndex(0); 
  return temp;
}


