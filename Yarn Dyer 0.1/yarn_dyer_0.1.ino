// This #include statement was automatically added by the Particle IDE.
#include "pid.h"

// This #include statement was automatically added by the Particle IDE.
#include "OneWire.h"

//thermometer variables - from library example
OneWire ds(D0);  // on pin D0 (a 4.7K resistor is necessary)

// PID variables - from library example
#define RelayPin 1
//Define Variables we'll be connecting to
double Setpoint, Output;
int WindowSize = 5000;
int setTimer = 90;
int state = 0;
int startTime, runTime, timerProgress;
unsigned long windowStartTime;
double temperature;
boolean completeFlag = false;
boolean start = false;

//Specify the links and initial tuning parameters
PID myPID(&temperature, &Output, &Setpoint,2,5,1, PID::DIRECT);



void setup() {

// thermometer setup - from library example
Serial.begin(57600);

// PID setup - from library example
windowStartTime = millis();

//initialize the variables we're linked to
Setpoint = 100;

//tell the PID to range between 0 and the full window size
myPID.SetOutputLimits(0, WindowSize);

//turn the PID on
myPID.SetMode(PID::AUTOMATIC);

// expose variables to the cloud
Particle.variable("temp", temperature);
Particle.variable("setTemp", Setpoint);
Particle.variable("state", state);
Particle.variable("timer", setTimer);
Particle.variable("relayOn", RelayPin);
Particle.variable("complete", completeFlag);

Particle.function("userInput", userInput);

}

void loop() {

  //every state will need to know the temp
  temperature = thermometerRead();

  // state machine
  // each case represents a machine state
  // user changes state using the changeState function
  switch (state) {
    case 0: // inital state on startup
    // user will change variables via REST api - just loop for input

    // user can set start value to begin heating and timer
    if (start == true) {
      startTime = millis();
      runTime = setTimer * 60 * 1000;
      state = 2;
    }
    break;

    case 1: // reset state
    // return all settings to default values
    setTimer = 90;
    Setpoint = 100;
    completeFlag = false;
    digitalWrite(RelayPin,LOW);
    state = 0;
    break;

    case 2: // heating state
    // check for cancel
    if (start == false) {
      state = 3;
      digitalWrite(RelayPin,LOW);
      completeFlag = true;
    }

    // check time
    timerProgress = millis() - startTime;
    if (timerProgress > runTime) { // finish heating if timer is up
      state = 3;
      digitalWrite(RelayPin,LOW);
      completeFlag = true;
      start = false;
    }

    // take PID measurements
    myPID.Compute();

    // switch relay output based on proportional window
    if(millis() - windowStartTime>WindowSize)
    { //time to shift the Relay Window
      windowStartTime += WindowSize;
    }

    if(Output < millis() - windowStartTime) digitalWrite(RelayPin,HIGH);
    else digitalWrite(RelayPin,LOW);

    // output values to cloud for monitoring
    //Particle.publish("temperature", String(Input));
    //Particle.publish("timerProgress", String(timerProgress));
    break;

    case 3: // finish state
    // loop here so the user can retrieve info


    break;

    default: // probably a bad input, revert to state s0
    state = 0;
  }

}

static int userInput(String input) {
  // user sends control string to change values
  if (input=="s0") {state = 0; return 0;}
  else if (input=="s1") {state = 1;return 1;}
  else if (input=="s2") {state = 2;return 2;}
  else if (input=="s3") {state = 3;return 3;}
  else if (input=="tempUp") {Setpoint++;return 4;}
  else if (input=="tempDown") {Setpoint--;return 5;}
  else if (input=="timeUp") {setTimer++;return 6;}
  else if (input=="timeDown") {setTimer--;return 7;}
  else if (input=="start") {start = true;return 8;}
  else if (input=="cancel") {start = false;return 9;}
}

static double thermometerRead() {
  // thermometer read - from library example
  byte i;
    byte present = 0;
    byte type_s;
    byte data[12];
    byte addr[8];
    float celsius;

    if ( !ds.search(addr)) {
      Serial.println("No more addresses.");
      Serial.println();
      ds.reset_search();
      delay(250);
      return celsius;
    }

    Serial.print("ROM =");
    for( i = 0; i < 8; i++) {
      Serial.write(' ');
      Serial.print(addr[i], HEX);
    }

    if (OneWire::crc8(addr, 7) != addr[7]) {
        Serial.println("CRC is not valid!");
        return celsius;
    }
    Serial.println();

    // the first ROM byte indicates which chip
    switch (addr[0]) {
      case 0x10:
        Serial.println("  Chip = DS18S20");  // or old DS1820
        type_s = 1;
        break;
      case 0x28:
        Serial.println("  Chip = DS18B20");
        type_s = 0;
        break;
      case 0x22:
        Serial.println("  Chip = DS1822");
        type_s = 0;
        break;
      default:
        Serial.println("Device is not a DS18x20 family device.");
        return celsius;
    }

    ds.reset();
    ds.select(addr);
    ds.write(0x44, 1);        // start conversion, with parasite power on at the end

    delay(1000);     // maybe 750ms is enough, maybe not
    // we might do a ds.depower() here, but the reset will take care of it.

    present = ds.reset();
    ds.select(addr);
    ds.write(0xBE);         // Read Scratchpad

    Serial.print("  Data = ");
    Serial.print(present, HEX);
    Serial.print(" ");
    for ( i = 0; i < 9; i++) {           // we need 9 bytes
      data[i] = ds.read();
      Serial.print(data[i], HEX);
      Serial.print(" ");
    }
    Serial.print(" CRC=");
    Serial.print(OneWire::crc8(data, 8), HEX);
    Serial.println();

    // Convert the data to actual temperature
    // because the result is a 16 bit signed integer, it should
    // be stored to an "int16_t" type, which is always 16 bits
    // even when compiled on a 32 bit processor.
    int16_t raw = (data[1] << 8) | data[0];
    if (type_s) {
      raw = raw << 3; // 9 bit resolution default
      if (data[7] == 0x10) {
        // "count remain" gives full 12 bit resolution
        raw = (raw & 0xFFF0) + 12 - data[6];
      }
    } else {
      byte cfg = (data[4] & 0x60);
      // at lower res, the low bits are undefined, so let's zero them
      if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
      else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
      else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
      //// default is 12 bit resolution, 750 ms conversion time
    }
    celsius = (double)raw / 16.0;

    return celsius;
}
