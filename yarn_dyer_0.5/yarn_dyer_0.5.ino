

/*  Yarn Dyer 0.5
 *  
 *  States
 *  0: Start
 *  1: Setup - Temp
 *  2: Setup - Time
 *  3: Setup - Run
 *  4: Setup - Reset
 *  5: Setup - Change Temp
 *  6: Setup - Change Time
 *  7: Run - Select Pause
 *  8: Run - Select Cancel
 *  9: Pause
 *  10: Cancel
 *  11: Complete
 *  12: Reset
 */
 
#include <Encoder.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <LiquidCrystal.h>
#include <RBD_Button.h>
#include <RBD_Timer.h>
 
// set up onewire interface for temp sensor
#define ONE_WIRE_BUS 1
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// set up variables
int machineState, lastDebounceTime;
long oldPosition  = -999;
unsigned long setTime, setTimeMins, currentTime, startupTimeStart, elapsedStartupTime, blinkTimeStart, blinkTime;
float setTemp, currentTemp;
boolean blinkOn, encoderUp, encoderDown, buttonState;

// add some constants
const int relayPin = 1;
const int encoderPushPin = 4;
const int debounceTime = 50;
const int startupTime = 3000; // in millis
const int defaultSetTimeMins = 1; // in min
const int defaultSetTemp = 22; // in C

// create encoder
Encoder myEnc(2,3);

// create button
RBD::Button button(encoderPushPin); // input_pullup by default

// custom glyphs for the LCD
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
  
  // start display
  lcd.begin(16, 2);

  // set up custom LCD icons
  lcd.createChar(1, thermometer);
  lcd.createChar(2, degrees);
  lcd.createChar(3, clock);
  
  // start the temp sensor library
  sensors.begin();

  // button
  buttonState = false;

  blinkOn = false;
  blinkTimeStart = 0;
  blinkTime = 500; //in ms

  // set default run values
  setTimeMins = defaultSetTimeMins; // in mins
  setTime = setTimeMins * 60 * 1000; // in millis
  setTemp = defaultSetTemp; // in C

  pinMode(relayPin, OUTPUT);
  pinMode(encoderPushPin, INPUT);
  Serial.println("Start");

  startupTimeStart = millis();
}

void loop() {
  // check blink status
  if (millis() - blinkTimeStart > blinkTime) {
      blinkOn =! blinkOn;
      // Serial.println("Blink switched at " + millis());
      blinkTimeStart = millis();
  }

  // read rotary encoder input
  long newPosition = myEnc.read()/4;
  if (newPosition > oldPosition) {
    encoderUp = TRUE;
    Serial.println("Up");
  }
  if (newPosition < oldPosition) {
    encoderDown = TRUE;
    Serial.println("Down");
  }
  if (newPosition != oldPosition) {
    oldPosition = newPosition;
    Serial.println(newPosition);
  }

  
  // this block of code controls the state machine
  // state operations are abstracted to seperate classes for readability
  
  switch (machineState) {
    case 0: // start
      startupState();
    break;
    
    case 1: // setup - temp
      setupMenuTempState();
    break;
    
    case 2: // Setup - Time
      setupMenuTimeState();
    break;
    
    case 3: // Setup - Run
      setupMenuRunState();   
    break;
    
    case 4: // Setup - Reset
      setupMenuResetState();
    break;
    
    case 5: // Setup - Change Temp
      setupMenuChangeTempState();
    break;
    
    case 6: // Setup - Change Time
      setupMenuChangeTimeState();
    break;

    case 7: // Run - Select Pause
      runMenuPauseState();
    break;
    
    case 8:  // Run - Select Cancel
      runMenuCancelState();
    break;
    
    case 9: // Pause
      pauseState();
    break;
    
    case 10: // Cancel
      cancelState();
    break;
    
    case 11: // Complete
      completeState();
    break;
    
    case 12: // reset
      resetState();
    break;
    
    default:
    // this shouldn't happen, but if it does: 
    // set state to start
    
    break;
  }

  
  // reset encoder states
  encoderUp = FALSE;
  encoderDown = FALSE;
}

// ************************************************
// Startup (State 0)
// ************************************************
void startupState() {
  // display startup screen, showing version info
    lcd.setCursor(1,0);
    lcd.print("Yarn Dyer v0.5");
    // Print a message to the LCD.
    lcd.setCursor(0,1);
    lcd.print("Live Fast Dye Yarn");
    
    // check elapsed time
    elapsedStartupTime = millis();
    
    // if elapsedTime has reach bootTime, move to setup state
    if ((elapsedStartupTime-startupTimeStart) > startupTime) {
      machineState = 1;
      lcd.clear();
    }
}

// ************************************************
// Setup - Temperature (State 1)
// ************************************************
void setupMenuTempState() {
    // display temp icon
    lcd.setCursor(0,0);
    lcd.write(1);

    // insert cursor
    lcd.setCursor(1,0);
    lcd.print(">");
    
    // temp
    lcd.setCursor(2,0);
    lcd.print(String(setTemp, 0));
    
    //spacing 
    lcd.setCursor(4,0);
    lcd.write(2); // degrees symbol
    lcd.setCursor(5,0);
    lcd.print("C");
    
    // display clock icon
    lcd.setCursor(10,0);
    lcd.write(3); // clock symbol
    
    // display time
    lcd.setCursor(12,0);
    lcd.print(setTimeMins);
    lcd.setCursor(15,0);
    lcd.print("m");     

    // menu line
    lcd.setCursor(1,1);
    lcd.print("Start     Reset");
    // listen for user input
    // scroll forward, move to Setup - Time (2)
    if (encoderUp) {
      Serial.println("Moving to Setup - Time");
      machineState = 2;
      // blank out cursor
      lcd.setCursor(1,0);
      lcd.print(" ");
    }
    
    // scroll back, move to Setup - Reset (4)
    if (encoderDown) {
      Serial.println("Moving to Setup - Reset");
      machineState = 4;
      // blank out cursor
      lcd.setCursor(1,0);
      lcd.print(" ");
    }
    
    // enter, move to Setup - Change Temp (5)
    if (button.onPressed()) {
        Serial.println("Moving to Setup - Change Temp");
        machineState = 5;
    }
}

// ************************************************
// Setup - Time (State 2)
// ************************************************
void setupMenuTimeState() {
    // display temp icon
    lcd.setCursor(0,0);
    lcd.write(1);

    //temp
    lcd.setCursor(2,0);
    lcd.print(String(setTemp, 0));
    
    //spacing 
    lcd.setCursor(4,0);
    lcd.write(2); // degrees symbol
    lcd.setCursor(5,0);
    lcd.print("C");
    
    // display clock icon
    lcd.setCursor(10,0);
    lcd.write(3); // clock symbol
    
    // insert cursor
    lcd.setCursor(11,0);
    lcd.print(">");
    
    // display time
    lcd.setCursor(12,0);
    lcd.print(setTimeMins);
    lcd.setCursor(15,0);
    lcd.print("m");     

    // menu line
    lcd.setCursor(1,1);
    lcd.print("Start     Reset");
    
    // listen for user input
    // scroll forward, move to Setup - Run (3) 
    if (encoderUp) {
      Serial.println("Moving to Setup - Run");
      machineState = 3;
      // blank out cursor
      lcd.setCursor(11,0);
      lcd.print(" ");
    }
    
    // scroll back, move to Setup - Temp (1)
    if (encoderDown) {
      Serial.println("Moving to Setup - Temp");
      machineState = 1;
      // blank out cursor
      lcd.setCursor(11,0);
      lcd.print(" ");
    }
    
    // enter, move to  Setup - Change Time (6) 
    if (button.onPressed()) {
        Serial.println("Moving to Setup - Change Time");
        machineState = 6;
    }
}
    
// ************************************************
// Setup - Run (State 3)
// ************************************************
void setupMenuRunState() {    
    // display temp icon
    lcd.setCursor(0,0);
    lcd.write(1);
    
    //temp
    lcd.setCursor(2,0);
    lcd.print(String(setTemp, 0));
    
    //spacing 
    lcd.setCursor(4,0);
    lcd.write(2); // degrees symbol
    lcd.setCursor(5,0);
    lcd.print("C");
    
    // display clock icon
    lcd.setCursor(10,0);
    lcd.write(3); // clock symbol
    
    // display time
    lcd.setCursor(12,0);
    lcd.print(setTimeMins);
    lcd.setCursor(15,0);
    lcd.print("m");     

    // insert blinking cursor
    lcd.setCursor(0,1);
    lcd.print(">");
    
    // menu line
    lcd.setCursor(1,1);
    lcd.print("Start     Reset");
    
    // listen for user input
    // scroll forward, move to Setup - Reset (4)
    if (encoderUp) {
      Serial.println("Moving to Setup - Reset");
      machineState = 4;
      // blank out cursor
      lcd.setCursor(0,1);
      lcd.print(" ");
    }
    
    // scroll back, move to Setup - Time (2)
    if (encoderDown) {
      Serial.println("Moving to Setup - Time");
      machineState = 2;
      // blank out cursor
      lcd.setCursor(0,1);
      lcd.print(" ");
    }
        
    // enter, move to Run - Select Pause (7)
    if (button.onPressed()) {
        Serial.println("Moving to Run - Select Pause");
        machineState = 7;
        lcd.clear();
    }
}

// ************************************************
// Setup - Reset (State 4)
// ************************************************
void setupMenuResetState() {
    // display temp icon
    lcd.setCursor(0,0);
    lcd.write(1);
    
    //temp
    lcd.setCursor(2,0);
    lcd.print(String(setTemp, 0));
    
    //spacing 
    lcd.setCursor(4,0);
    lcd.write(2); // degrees symbol
    lcd.setCursor(5,0);
    lcd.print("C");
    
    // display clock icon
    lcd.setCursor(10,0);
    lcd.write(3); // clock symbol
    
    // display time
    lcd.setCursor(12,0);
    lcd.print(setTimeMins);
    lcd.setCursor(15,0);
    lcd.print("m");     

    // menu line
    lcd.setCursor(1,1);
    lcd.print("Start");
    
    // insert cursor
    lcd.setCursor(10,1);
    lcd.print(">");

    // reset
    lcd.setCursor(11,1);
    lcd.print("Reset");
        
    // listen for user input
    // scroll forward, move to Setup - Temp (1)
    if (encoderUp) {
      Serial.println("Moving to Setup - Temp");
      machineState = 1;
      lcd.setCursor(10,1);
      lcd.print(" ");
    }
    
    // scroll back, move to Setup - Run (3)
    if (encoderDown) {
      Serial.println("Moving to Setup - Run");
      machineState = 3;
      lcd.setCursor(10,1);
      lcd.print(" ");
    }

    // enter, move to Reset (12)
    if (button.onPressed()) {
      Serial.println("Moving to Reset");
      machineState = 12;
      lcd.clear();
    }
}
    
// ************************************************
// Setup - Change Temperature (State 5)
// ************************************************
void setupMenuChangeTempState() {
    // display temp icon
    lcd.setCursor(0,0);
    lcd.write(1);

    // insert blinking cursor
    lcd.setCursor(1,0);
    if (blinkOn) {lcd.print(">");} else {lcd.print(" ");}
    
    // temp
    lcd.setCursor(2,0);
    lcd.print(String(setTemp, 0));
    
    // spacing 
    lcd.setCursor(4,0);
    lcd.write(2); // degrees symbol
    lcd.setCursor(5,0);
    lcd.print("C");
    
    // display clock icon
    lcd.setCursor(10,0);
    lcd.write(3); // clock symbol
    
    // display time
    lcd.setCursor(12,0);
    lcd.print(setTimeMins);
    lcd.setCursor(15,0);
    lcd.print("m");     

    // menu line
    lcd.setCursor(1,1);
    lcd.print("Start     Reset");
    
    // listen for user input
    // scroll forward, increase setTemp 
    if (encoderUp) {
      Serial.println("Temp Up");
      setTemp++;
    }
    
    // scroll back, decrease setTemp
    if (encoderDown) {
      Serial.println("Temp Down");
      setTemp--;
    }
    
    // enter, move to Setup - Temp (state 1)
    if (button.onPressed()) {
        Serial.println("Moving to Setup - Temp");
        machineState = 1;
    }
}

// ************************************************
// Setup - Change Time (State 6)
// ************************************************    
void setupMenuChangeTimeState() {
    // display temp icon
    lcd.setCursor(0,0);
    lcd.write(1);
    
    // temp
    lcd.setCursor(2,0);
    lcd.print(String(setTemp, 0));
    
    // spacing 
    lcd.setCursor(4,0);
    lcd.write(2); // degrees symbol
    lcd.setCursor(5,0);
    lcd.print("C");
    
    // display clock icon
    lcd.setCursor(10,0);
    lcd.write(3); // clock symbol

    // insert blinking cursor
    lcd.setCursor(11,0);
    if (blinkOn) {lcd.print(">");} else {lcd.print(" ");}
    
    // time
    lcd.setCursor(12,0);
    lcd.print(setTimeMins);
    lcd.setCursor(15,0);
    lcd.print("m");     

    // menu line
    lcd.setCursor(1,1);
    lcd.print("Start     Reset");
    
    // listen for user input
    // scroll forward, increase setTime
    if (encoderUp) {
      Serial.println("Time Up");
      setTimeMins++;
    }
    
    // scroll back, decrease setTime
    if (encoderDown) {
      Serial.println("Time Down");
      setTimeMins--;
    }
    
    // enter, move to Setup - Time (state 2)
    if (button.onPressed()) {
        Serial.println("Moving to Setup - Time");
        machineState = 2;
    }
}

// ************************************************
// Run - Select Pause (State 7)
// ************************************************
void runMenuPauseState() {
    // display time and temp on screen
    lcd.setCursor(0,0);
    lcd.print("Run - Select Pause State");
    
    // listen for user input
    // scroll forward, move to Run - Select Cancel
    if (encoderUp) {
      Serial.println("Moving to Run - Select Cancel");
      machineState = 8;
      //lcd.setCursor(10,1);
      //lcd.print(" ");
    }
    
    // scroll back, move to Setup - Run (3)
    if (encoderDown) {
      Serial.println("Moving to Run - Select Cancel");
      machineState = 8;
      //lcd.setCursor(10,1);
      //lcd.print(" ");
    }
    
    // if user presses enter, move to Pause (9)
    if (button.onPressed()) {
        Serial.println("Moving to Pause");
        machineState = 9;
    }
    // turn off heater, store current time, break


    // check the time
    // if elapsedTime has reached setTime, move to Complete (11)
    // turn off heater, break

    // otherwise update the temperature
    // only run the update every 30 seconds
        
    // read temperature
    // check temperature against set temperature
    // if needed update relay
}
   
// ************************************************
// Run - Select Cancel (State 8)
// ************************************************
void runMenuCancelState() {

    // display time and temp on screen
    lcd.setCursor(0,0);
    lcd.print("Run - Select Cancel State");

    // scroll forward, move to Run - Select Pause (7)
    if (encoderUp) {
      Serial.println("Moving to Run - Select Pause");
      machineState = 7;
      //lcd.setCursor(10,1);
      //lcd.print(" ");
    }
    
    // scroll back, move to Run - Select Pause (7)
    if (encoderDown) {
      Serial.println("Moving to Run - Select Pause");
      machineState = 7;
      //lcd.setCursor(10,1);
      //lcd.print(" ");
    }
    
    // if user presses enter, move to Cancel (10)
    if (button.onPressed()) {
        Serial.println("Moving to Cancel");
        machineState = 10;
    }
    // turn off heater, store total time, break
}

// ************************************************
// Pause (State 9)
// ************************************************
void pauseState() {
    // display time elapsed, current temp
    // listen for user input to resume
    lcd.setCursor(0,0);
    lcd.print("Pause State");

    // if user presses enter, move to Run - Select Pause (7)
    if (button.onPressed()) {
        Serial.println("Moving to Run - Select Pause");
        machineState = 7;
    }
    // update startTime, break
}
    
// ************************************************
// Cancel (State 10)
// ************************************************
void cancelState() {
    // display time elapsed, current temp
    // listen for user input to reset
    lcd.setCursor(0,0);
    lcd.print("Cancel");

    // if user presses enter, move to Reset (12)
    if (button.onPressed()) {
        Serial.println("Moving to Reset");
        machineState = 12;
    }
}

// ************************************************
// Complete (State 11)
// ************************************************
void completeState() {
    // display time elapsed, current temp
    // listen for user input to reset
    lcd.setCursor(0,0);
    lcd.print("Complete");

    // if user presses enter, move to Reset (12)
    if (button.onPressed()) {
        Serial.println("Moving to Reset");
        machineState = 12;
    }
}
    
// ************************************************
// Reset (State 12)
// ************************************************
void resetState() {
    // set default run values
    setTimeMins = defaultSetTimeMins; // in mins
    setTime = setTimeMins * 60 * 1000; // in millis
    setTemp = defaultSetTemp; // in C
    
    // move back to startup state
    Serial.println("Moving to Startup");
    machineState = 0;

    lcd.clear();

    startupTimeStart = millis();
}

float getTemp(void) {
  sensors.requestTemperatures(); 
  float temp = sensors.getTempCByIndex(0); 
  return temp;
}

// encoder reading. code from https://www.circuitsathome.com/mcu/programming/reading-rotary-encoder-on-arduino
/*int8_t read_encoder() {
  static int8_t enc_states[] = {0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0};
  static uint8_t old_AB = 0;
  old_AB <<= 2;                   //remember previous state
  old_AB |= ( ENC_PORT & 0x03 );  //add current state
  return ( enc_states[( old_AB & 0x0f )]);
}
*/

