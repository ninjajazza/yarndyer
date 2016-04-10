/*  Yarn Dyer 0.4
 *  
 *  States
 *  0: Start
 *  1: Setup - setTemp
 *  2: Setup - setTime
 *  3: Setup - setMenu
 *  4: Run
 *  3: Pause
 *  4: Cancel
 *  5: Complete
 *  6: Reset
 */

#include <OneWire.h>
#include <DallasTemperature.h>
#include <LiquidCrystal.h>

// set up onewire interface for temp sensor
#define ONE_WIRE_BUS 2
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// set up variables
int machineState;
unsigned long setTime, setTimeMins, currentTime, elapsedBootTime, blinkTimeStart, blinkTime;
float setTemp, currentTemp;
boolean blinkOn; 

// add some constants
const int relayPin = 1;
const int bootTime = 3000; // in millis
const int defaultSetTimeMins = 1; // in min
const int defaultSetTemp = 22; // in C


byte degrees[8] = {0x06, 0x09, 0x09, 0x06, 0x00, 0x00, 0x00, 0x00};
byte thermometer[8] = {0x04, 0x0A, 0x0A, 0x0E, 0x0E, 0x1F, 0x1F, 0x0E};
byte clock[8] = {0x00, 0x0E, 0x15, 0x15, 0x13, 0x0E, 0x00, 0x00};


// set up display
//                RS E  D4  D5 D6 D7
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

void setup() {
  // set machine start state
  machineState = 0;
  
  // start serial port
  Serial.begin(9600);
  Serial.println("Dallas Temperature IC Control Library Demo");
  
  // start display
  lcd.begin(16, 2);

  // set up custom LCD icons
  lcd.createChar(1, thermometer);
  lcd.createChar(2, degrees);
  lcd.createChar(3, clock);
  
  // start the temp sensor library
  sensors.begin();

  blinkOn = false;
  blinkTimeStart = 0;
  blinkTime = 1000; //in ms

  // set default run values
  setTimeMins = defaultSetTimeMins; // in mins
  setTime = setTimeMins * 60 * 1000; // in millis
  setTemp = defaultSetTemp; // in C

  pinMode(relayPin, OUTPUT);
}

void loop() {
  switch (machineState) {
    
    case 0: // start
    // display startup screen, showing version info
    lcd.setCursor(1,0);
    lcd.print("Yarn Dyer v0.4");
    // Print a message to the LCD.
    lcd.setCursor(3,1);
    lcd.print("31/03/2016");
    
    // check elapsed time
    elapsedBootTime = millis();
    
    // if elapsedTime has reach bootTime, move to setup state
    if (elapsedBootTime > bootTime) {
      machineState = 1;
      lcd.clear();
    }
    break;
    
    case 1: // setup
    // display preset time and temp on screen
    // display temp icon
    lcd.setCursor(0,0);
    lcd.write(1);
    
    //temp
    lcd.setCursor(2,0);
    if (blinkOn) {
    lcd.print(String(setTemp, 0));
    } else {
    lcd.print("   ");  
    }
    
    //spacing 
    lcd.setCursor(4,0);
    lcd.write(2);
    lcd.setCursor(5,0);
    lcd.print("C");
    
    // display clock icon
    lcd.setCursor(10,0);
    lcd.write(3);
    //time
    lcd.setCursor(12,0);
    lcd.print(setTimeMins);
    lcd.setCursor(15,0);
    lcd.print("m");     
    

    // menu line
    lcd.setCursor(0,1);
    lcd.print("Start");
    
    // listen for user input

    // if user presses start, move to start state, break

    // if user presses reset, move to reset state, break

    // check blink timer
    if (millis() - blinkTimeStart > blinkTime) {
      blinkOn =! blinkOn;
      Serial.println("Blink switched at " + millis());
      blinkTimeStart = millis();
    }
    
    break;
    
    case 2: // run
    // display time and temp on screen

    // listen for user input to pause, cancel

    // if user presses pause, enter pause state
    // turn off heater, store current time, break

    // if user presses cancel, enter cancel state
    // turn off heater, store total time, break

    // check the time
    // if elapsedTime has reached setTime, enter complete state
    // turn off heater, break

    // otherwise update the temperature
    // only run the update every 30 seconds
        
    // read temperature
    // check temperature against set temperature
    // if needed update relay

    break;
    
    case 3: // pause
    // display time elapsed, current temp
    // listen for user input to resume, reset

    // if user resumes, enter run state
    // update startTime, break

    // if user resets, enter reset state
        
    break;
    
    case 4: // cancel
    // display time elapsed, current temp
    // listen for user input to reset

    // if user resets, enter reset state, break
    
    break;
    
    case 5: // complete
    // display time elapsed, current temp
    // listen for user input to reset

    // if user resets, enter reset state, break
    
    break;
    
    case 6: // reset
    // set time values back to default

    // set state to start
    
    break;
    
    default:
    // this shouldn't happen, but if it does: 
    // set state to start
    
    break;
  }
}


float getTemp(void) {
  sensors.requestTemperatures(); 
  float temp = sensors.getTempCByIndex(0); 
  return temp;
}

