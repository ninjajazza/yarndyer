/*  Yarn Dyer

    States
    0: Start
    1: Setup - Temp
    2: Setup - Time
    3: Setup - Run
    4: Setup - Reset
    5: Setup - Change Temp
    6: Setup - Change Time
    7: Run
    8: Cancel
    9: Complete
    10: Reset
*/

#include <Encoder.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <LiquidCrystal.h>
#include <RBD_Button.h>
#include <RBD_Timer.h>

// set up onewire interface for temp sensor
#define ONE_WIRE_BUS 5
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// set up timers
RBD::Timer startupTimer;
RBD::Timer blinkTimer;
RBD::Timer tempCheckTimer;
RBD::Timer heatingTimer;

// set up variables
int machineState;
long oldPosition  = -999;
unsigned long setTime, setTimeMins, currentTime, pausedTime, elapsedHeatingTime, displayHeatingTime, blinkTime;
float setTemp, currentTemp;
boolean blinkOn, encoderUp, encoderDown, buttonState;

// add some constants
const int relayPin = 6;
const int encoderPushPin = 4;
const int debounceTime = 50;
const int startupTime = 3000; // in millis
const int defaultSetTimeMins = 1; // in min
const int defaultSetTemp = 22; // in C

const String version = "v0.7";

// create encoder
Encoder myEnc(2, 3);

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
  sensors.setResolution(9); //reduce resolution to speed things up

  // button
  buttonState = false;

  //set up timers
  startupTimer.setTimeout(3000); // period to display boot screen
  blinkTimer.setHertz(2); // frequency of blinking timer
  tempCheckTimer.setHertz(1); // frequency of temperature checks
  //heatingTimer;

  blinkOn = false;

  // set default run values
  setTimeMins = defaultSetTimeMins; // in mins
  setTime = setTimeMins * 60 * 1000; // in millis
  setTemp = defaultSetTemp; // in C

  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin,LOW);
  pinMode(encoderPushPin, INPUT);
  Serial.println("Start");

  // start the startup timer
  startupTimer.restart();

}

void loop() {
  // check blink status
  if (blinkTimer.onRestart()) {
    blinkOn = ! blinkOn;
  }

  // read rotary encoder input
  long newPosition = myEnc.read() / 4;
  if (newPosition > oldPosition) {
    encoderUp = TRUE;
    //Serial.println("Up");
  }
  if (newPosition < oldPosition) {
    encoderDown = TRUE;
    //Serial.println("Down");
  }
  if (newPosition != oldPosition) {
    oldPosition = newPosition;
    //Serial.println(newPosition);
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

    case 7:  // Run
      runMenuCancelState();
      break;

    case 8: // Cancel
      cancelState();
      break;

    case 9: // Complete
      completeState();
      break;

    case 10: // reset
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
  lcd.setCursor(1, 0);
  lcd.print("Yarn Dyer " + version);
  // Print a message to the LCD.
  lcd.setCursor(0, 1);
  lcd.print("livefast dyeyarn");

  // check timer, move to setup state if required
  if (startupTimer.isExpired()) {
    machineState = 1;
    lcd.clear();
  }
}

// ************************************************
// Setup - Temperature (State 1)
// ************************************************
void setupMenuTempState() {

  setupMenu(setTemp, setTimeMins, 1, 0, false);

  // listen for user input
  // scroll forward, move to Setup - Time (2)
  if (encoderUp) {
    machineState = 2;
    Serial.println("Moving to " + String(machineState));
    // blank out cursor
    lcd.setCursor(1, 0);
    lcd.print(" ");
  }

  // scroll back, move to Setup - Reset (4)
  if (encoderDown) {
    machineState = 4;
    Serial.println("Moving to " + String(machineState));
    // blank out cursor
    lcd.setCursor(1, 0);
    lcd.print(" ");
  }

  // enter, move to Setup - Change Temp (5)
  if (button.onPressed()) {
    machineState = 5;
    Serial.println("Moving to " + String(machineState));
  }
}

// ************************************************
// Setup - Time (State 2)
// ************************************************
void setupMenuTimeState() {

  setupMenu(setTemp, setTimeMins, 11, 0, false);

  // listen for user input
  // scroll forward, move to Setup - Run (3)
  if (encoderUp) {
    machineState = 3;
    Serial.println("Moving to " + String(machineState));
    // blank out cursor
    lcd.setCursor(11, 0);
    lcd.print(" ");
  }

  // scroll back, move to Setup - Temp (1)
  if (encoderDown) {
    machineState = 1;
    Serial.println("Moving to " + String(machineState));
    // blank out cursor
    lcd.setCursor(11, 0);
    lcd.print(" ");
  }

  // enter, move to  Setup - Change Time (6)
  if (button.onPressed()) {
    machineState = 6;
    Serial.println("Moving to " + String(machineState));
  }
}

// ************************************************
// Setup - Run (State 3)
// ************************************************
void setupMenuRunState() {

  setupMenu(setTemp, setTimeMins, 0, 1, false);

  // listen for user input
  // scroll forward, move to Setup - Reset (4)
  if (encoderUp) {
    machineState = 4;
    Serial.println("Moving to " + String(machineState));
    // blank out cursor
    lcd.setCursor(0, 1);
    lcd.print(" ");
  }

  // scroll back, move to Setup - Time (2)
  if (encoderDown) {
    machineState = 2;
    Serial.println("Moving to " + String(machineState));
    // blank out cursor
    lcd.setCursor(0, 1);
    lcd.print(" ");
  }

  // enter, move to Run (7)
  if (button.onPressed()) {
    machineState = 7;
    Serial.println("Moving to " + String(machineState));
    lcd.clear();

    setTime = setTimeMins * 60UL * 1000UL; // in millis
    heatingTimer.setTimeout(setTime); // set the heating timer
    heatingTimer.restart(); // begin the timer

    //tempCheckTimer.restart(); // begin the periodic timer to run the temp check
  }
}

// ************************************************
// Setup - Reset (State 4)
// ************************************************
void setupMenuResetState() {

  setupMenu(setTemp, setTimeMins, 10, 1, false);

  // listen for user input
  // scroll forward, move to Setup - Temp (1)
  if (encoderUp) {
    machineState = 1;
    Serial.println("Moving to " + String(machineState));
    lcd.setCursor(10, 1);
    lcd.print(" ");
  }

  // scroll back, move to Setup - Run (3)
  if (encoderDown) {
    machineState = 3;
    Serial.println("Moving to " + String(machineState));
    lcd.setCursor(10, 1);
    lcd.print(" ");
  }

  // enter, move to Reset (12)
  if (button.onPressed()) {
    machineState = 12;
    Serial.println("Moving to " + String(machineState));
    lcd.clear();
  }
}

// ************************************************
// Setup - Change Temperature (State 5)
// ************************************************
void setupMenuChangeTempState() {

  setupMenu(setTemp, setTimeMins, 1, 0, true);

  // listen for user input
  // scroll forward, increase setTemp
  if (encoderUp) {
    setTemp++;
    setTemp = constrain(setTemp, 10, 110);
    Serial.println("Temp Up");
  }

  // scroll back, decrease setTemp
  if (encoderDown) {
    setTemp--;
    setTemp = constrain(setTemp, 10, 110);
    Serial.println("Temp Down");
  }

  // enter, move to Setup - Temp (state 1)
  if (button.onPressed()) {
    machineState = 1;
    Serial.println("Moving to " + String(machineState));
  }
}

// ************************************************
// Setup - Change Time (State 6)
// ************************************************
void setupMenuChangeTimeState() {

  setupMenu(setTemp, setTimeMins, 11, 0, true);

  // listen for user input
  // scroll forward, increase setTime
  if (encoderUp) {
    setTimeMins++;
    setTimeMins = constrain(setTimeMins, 1, 240);
    Serial.println("Time Up");
  }

  // scroll back, decrease setTime
  if (encoderDown) {
    setTimeMins--;
    setTimeMins = constrain(setTimeMins, 1, 240);
    Serial.println("Time Down");
  }

  // enter, move to Setup - Time (state 2)
  if (button.onPressed()) {
    machineState = 2;
    Serial.println("Moving to " + String(machineState));
  }
}

// ************************************************
// Run (State 7)
// ************************************************
void runMenuCancelState() {

  // check the time
  currentTime = millis();
  Serial.println("Current Time: " + String(currentTime));

  elapsedHeatingTime = heatingTimer.getValue();

  Serial.println("Elapsed Heating Time: " + String(elapsedHeatingTime));

  // update the temperature
  if (tempCheckTimer.onRestart()) {
    currentTemp = getTemp();
  }

  // check temperature against set temperature and set relay
  if (currentTemp < setTemp ) {
    digitalWrite(relayPin, HIGH);
  } else {
    digitalWrite(relayPin, LOW);
  }

  displayHeatingTime = elapsedHeatingTime / 60000;
  Serial.println("Display Heating Time: " + String(displayHeatingTime));

  runMenu(currentTemp, displayHeatingTime, setTimeMins);

  // cancel option
  lcd.setCursor(10, 1);
  lcd.print("Cancel");

  // cursor
  lcd.setCursor(9, 1);
  if (blinkOn) {
    lcd.print(">");
  } else {
    lcd.print(" ");
  }

  // if user presses enter, move to Cancel (8)
  if (button.onPressed()) {
    machineState = 8;
    Serial.println("Moving to " + machineState);
    lcd.clear();

    // stop timer
    heatingTimer.stop();

    // turn off relay
    digitalWrite(relayPin, LOW);
  }

  // if elapsedTime has reached setTime, move to Complete (9)
  if (heatingTimer.isExpired()) {
    machineState = 9;
    Serial.println("Moving to " + machineState);
    lcd.clear();
    displayHeatingTime = elapsedHeatingTime / 60000;
    // turn off relay
    digitalWrite(relayPin, LOW);
  }

}

// ************************************************
// Cancel (State 8)
// ************************************************
void cancelState() {
  // check the time
  currentTime = millis();
  Serial.println("Current Time: " + String(currentTime));

  elapsedHeatingTime = heatingTimer.getValue();

  Serial.println("Elapsed Heating Time: " + String(elapsedHeatingTime));

  // otherwise update the temperature
  if (tempCheckTimer.onRestart()) {
    currentTemp = getTemp();
  }

  runMenu(currentTemp, displayHeatingTime, setTimeMins);

  // cancelled
  lcd.setCursor(0, 1);
  lcd.print("Cancelled");

  // reset option
  lcd.setCursor(11, 1);
  lcd.print("Reset");

  // cursor
  lcd.setCursor(10, 1);
  if (blinkOn) {
    lcd.print(">");
  } else {
    lcd.print(" ");
  }

  // if user presses enter, move to Reset (10)
  if (button.onPressed()) {
    machineState = 10;
    Serial.println("Moving to " + machineState);
    lcd.clear();
  }
}

// ************************************************
// Complete (State 9)
// ************************************************
void completeState() {
  // check the time
  currentTime = millis();
  Serial.println("Current Time: " + String(currentTime));

  elapsedHeatingTime = heatingTimer.getValue();

  Serial.println("Elapsed Heating Time: " + String(elapsedHeatingTime));

  // otherwise update the temperature
  if (tempCheckTimer.onRestart()) {
    currentTemp = getTemp();
  }

  runMenu(currentTemp, displayHeatingTime, setTimeMins);

  // complete
  lcd.setCursor(0, 1);
  lcd.print("Complete");

  // reset option
  lcd.setCursor(11, 1);
  lcd.print("Reset");

  // cursor
  lcd.setCursor(10, 1);
  if (blinkOn) {
    lcd.print(">");
  } else {
    lcd.print(" ");
  }

  // if user presses enter, move to Reset (10)
  if (button.onPressed()) {
    machineState = 10;
    Serial.println("Moving to " + machineState);
    lcd.clear();
  }
}

// ************************************************
// Reset (State 10)
// ************************************************
void resetState() {

  // set default run values
  setTimeMins = defaultSetTimeMins; // in mins
  setTime = setTimeMins * 60UL * 1000UL; // in millis
  setTemp = defaultSetTemp; // in C

  // move back to startup state
  machineState = 0;
  Serial.println("Moving to " + machineState);

  lcd.clear();

  startupTimer.restart();
}


// ************************************************
// getTemp
// abstracts the code required to fetch the temperature
// ************************************************
float getTemp(void) {
  sensors.requestTemperatures();
  float temp = sensors.getTempCByIndex(0);
  Serial.println(temp);
  return temp;
}

// ************************************************
// formatIntValue
// formats a long value into a fixed length, right justified string
// ************************************************
String formatIntValue(long inputValue, int outputLength) {
  String outputValue = String(inputValue);
  int stringLength = outputValue.length();
  while (stringLength < outputLength) {
    outputValue = " " + outputValue;
    stringLength = outputValue.length();
  }
  return outputValue;
  Serial.println(outputValue);
}

// ************************************************
// formatFloatValue
// formats a long value into a fixed length, right justified string
// ************************************************
String formatFloatValue(float inputValue, int outputLength) {
  String outputValue = String(inputValue, 0);
  int stringLength = outputValue.length();
  while (stringLength < outputLength) {
    outputValue = " " + outputValue;
    stringLength = outputValue.length();
  }
  return outputValue;
  Serial.println(outputValue);
}

// ************************************************
// Display (Setup)
// the display screen for the setup menu
// pass in variables to configure behaviour of this screen
// ************************************************
void setupMenu(float displayTemp, int displayTime, int cursorRow, int cursorCol, boolean cursorBlinking) {
  // display temp icon
  lcd.setCursor(0, 0);
  lcd.write(1);

  // display temp
  lcd.setCursor(5, 0);
  lcd.write(2); // degrees symbol
  lcd.setCursor(2, 0);
  lcd.print(formatIntValue(displayTemp, 3));
  lcd.setCursor(6, 0);
  lcd.print("C");

  // display clock icon
  lcd.setCursor(10, 0);
  lcd.write(3); // clock symbol
  // display time
  lcd.setCursor(12, 0);
  lcd.print(formatIntValue(displayTime, 3));
  // display time formatting
  lcd.setCursor(15, 0);
  lcd.print("m");

  // menu line
  lcd.setCursor(1, 1);
  lcd.print("Start");

  lcd.setCursor(11, 1);
  lcd.print("Reset");

  // insert cursor
  lcd.setCursor(cursorRow, cursorCol);
  if (cursorBlinking) {
    if (blinkOn) {
      lcd.print(">");
    } else {
      lcd.print(" ");
    }
  } else {
    lcd.print(">");
  }
}

// ************************************************
// Display (Run)
// the display screen for the runp menu
// pass in variables to configure behaviour of this screen
// only takes up the top row
// ************************************************
void runMenu(float displayTemp, int progressTime, int runningTime) {
  // display temp icon
  lcd.setCursor(0, 0);
  lcd.write(1);

  // temp
  lcd.setCursor(1, 0);
  lcd.print(formatFloatValue(displayTemp, 3));

  // temp formatting
  lcd.setCursor(4, 0);
  lcd.write(2); // degrees symbol
  lcd.setCursor(5, 0);
  lcd.print("C");

  // display clock icon
  lcd.setCursor(7, 0);
  lcd.write(3); // clock symbol

  // display progress time
  lcd.setCursor(8, 0);
  lcd.print(formatIntValue(progressTime, 3));

  // display time formatting elements
  lcd.setCursor(11, 0);
  lcd.print("/");

  // display set time
  lcd.setCursor(12, 0);
  lcd.print(formatIntValue(runningTime, 3));
  lcd.setCursor(15, 0);
  lcd.print("m");


}

// ************************************************
// MachineStateName
// This function returns the description of a machine state when given the number of the state
// ************************************************
String machineStateName(int stateNumber) {
  switch (stateNumber) {
    case 0: // start
      return "Startup";
      break;

    case 1: // setup - temp
      return "Setup - Temperature";
      break;

    case 2: // Setup - Time
      return "Setup - Time";
      break;

    case 3: // Setup - Run
      return "Setup - Run";
      break;

    case 4: // Setup - Reset
      return "Setup - Reset";
      break;

    case 5: // Setup - Change Temp
      return "Setup - Change Temp";
      break;

    case 6: // Setup - Change Time
      return "Setup - Change Time";
      break;

    case 7:  // Run
      return "Run";
      break;

    case 8: // Cancel
      return "Cancel";
      break;

    case 9: // Complete
      return "Complete";
      break;

    case 10: // Reset
      return "Reset";
      break;
  }
}


