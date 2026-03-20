#include <Wire.h> //standrad library for I2C communication
#include <SevSeg.h>

SevSeg sevseg; //object sevseg is created
const byte SQW_PIN    = 2;  
const byte BUTTON_PIN = 3;   
// 7-segment wiring
const byte segPins[]   = {4, 5, 6, 7, 8, 9, 10, 11};
const byte digitPins[] = {A3, A2, A1, A0}; // D1 - D4 from right to left (D1 is connected to A3, however we use analog pins as digital ones)
const byte DS1307_ADDR = 0x68;
const byte TARGET = 10;
const unsigned long WIN_MS = 100;
const unsigned long DEBOUNCE_MS = 30; // only accept button change if it is stable for 30ms

#define RUN_START_SELFTEST 1

volatile unsigned long lastTickMs = 0; // saves the current millisecond into lastTickMs
volatile bool tickFlag = false;
enum State { IDLE, RUNNING, RESULT }; //state machine
State state = IDLE; //system begins with IDLE
byte counterVal = 0;
unsigned long targetTickMs = 0; // when the user presses the button, so it compares target time and press time
bool resultSuccess = false; //before the game starts there is no success, that's why we start from false.
bool buttonState = HIGH; //because we are using pull up resistor
bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;

// INTERRUPT
void onTick() { //function named onTick, void => returns nothing
  lastTickMs = millis(); // millis is used as a timestamp the momemnt when external RTC tick arrived
  tickFlag = true; // meaning a tick happened. 
}

// DS1307 
void ds1307EnsureRunning() {
  Wire.beginTransmission(DS1307_ADDR);// arduino starts to talk
  Wire.write((byte)0x00); // points to seconds register inside ds1307
  Wire.endTransmission();
  Wire.requestFrom(DS1307_ADDR, (byte)1); //ds1307 needs to send 1 byte to arduino
  if (Wire.available()) {
    byte sec = Wire.read(); //stores received byte into the variable sec
    if (sec & 0x80) { // bit 7, if it is 1 oscillator disabled, if it is 0 = enabled (0 x 80 = 10000000)
      sec &= 0x7F; // 0x7F is binary 01111111
      Wire.beginTransmission(DS1307_ADDR);
      Wire.write((byte)0x00);
      Wire.write(sec);
      Wire.endTransmission();
    }
  }
}

void ds1307SetSQW_1Hz() { //function to set square-wave output to 1Hz 
  Wire.beginTransmission(DS1307_ADDR);
  Wire.write((byte)0x07); // 0x07 is a register address that controls the square-wave output
  Wire.write((byte)0x10); // 1Hz square wave
  Wire.endTransmission();
}

// DISPLAY HELPERS 
void showCount(byte v) { //function will display the number v
  char buf[5] = "    "; //start with blank 4 digit display, 5 because of \0
  if (v < 10) {
    buf[0] = ' ';
    buf[1] = ' '; // first 2 positions are blank
    buf[2] = '0' + v; // showing a digit
    buf[3] = ' ';
  } else if (v == 10) {
    buf[0] = '1'; // 1o is written in first 2 positions
    buf[1] = '0';
    buf[2] = ' ';
    buf[3] = ' ';
  } else {
    buf[0] = buf[1] = buf[2] = buf[3] = '-';
  }
  // Flip for mirrored display
  char flipped[5]; // same as in the buf[5]
  flipped[0] = buf[3]; //last character from the buf => put it into the first position
  flipped[1] = buf[2];
  flipped[2] = buf[1];
  flipped[3] = buf[0];
  flipped[4] = '\0';
  sevseg.setChars(flipped);
}

void showResult(bool ok) { //ok = true (success), ok = false (failure)
  char buf[5];
  if (ok) { buf[0]='G'; buf[1]='O'; buf[2]='O'; buf[3]='D'; }
  else    { buf[0]='F'; buf[1]='A'; buf[2]='I'; buf[3]='L'; }
  char flipped[5];
  flipped[0] = buf[3]; // the same idea of flipping 
  flipped[1] = buf[2];
  flipped[2] = buf[1];
  flipped[3] = buf[0];
  flipped[4] = '\0';
  sevseg.setChars(flipped);
}

//  RESET 
void resetToIdle() {
  state = IDLE;
  counterVal = 0; // resets back to 0
  resultSuccess = false;
  showCount(0); //display shows 0
}

//  SELF TEST
void displaySelfTest() {
  const char* patterns[] = {"8   ", " 8  ", "  8 ", "   8"};
  for (int r=0;r<2;r++) { //running it 2 times
    for (int i=0;i<4;i++) { //running 4 times
      sevseg.setChars(patterns[i]); // using i=0,1,2,3 (postion on the display to fill up the 8s)
      unsigned long t0 = millis(); //stors current time in t0
      while (millis()-t0<400) sevseg.refreshDisplay(); //keep doing it until 400ms have passed
    }
  }
  sevseg.setChars("----");
  unsigned long t1 = millis();
  while (millis()-t1<300) sevseg.refreshDisplay(); // dashes (----) stay visibile for 300ms
}

// BUTTON 
bool buttonPressedEvent() {
  bool reading = !(PIND & (1<<3)); // digitalRead 3 using register arduino pin 3 belongs to port D, 1<<3 (strat from binary 00000001 and move left by 3 00001000, I onle care about bit3), 
  // we used ! because we use pull up logic, and need to change it
  bool pressed = false;
  if (reading != lastButtonState) lastDebounceTime = millis(); // is the current reading not equal (different from previous one?)
  if ((millis()-lastDebounceTime) > DEBOUNCE_MS) { //has the signal stayed unchanged for more than 30ms? => if yes, then the press is real
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState) pressed = true; 
    }
  }
  lastButtonState = reading; // we store the current reading to compare it with reading !=lastButtonState
  return pressed;
}

// SETUP 
void setup() {

  // Button input with pullup
  DDRD &= ~(1<<3); //sets pin 3 as input as it is my button pin
  PORTD |= (1<<3); // enables internal pull-up resistor on pin 3
  // SQW pin input with pullup
  DDRD &= ~(1<<2); // pin 2 as input as it receives SQW signal
  PORTD |= (1<<2); //pull-up resistor on pin 2
  Wire.begin(); // I2C interface start
  ds1307EnsureRunning();
  ds1307SetSQW_1Hz();
  bool resistorsOnSegments = true; //8 resistor near segment pins = true
  byte hardwareConfig = COMMON_CATHODE; //lights when high
  sevseg.begin(hardwareConfig, 4, (byte*)digitPins, (byte*)segPins, resistorsOnSegments);
  sevseg.setBrightness(70);
#if RUN_START_SELFTEST
  displaySelfTest();
#endif
  attachInterrupt(digitalPinToInterrupt(SQW_PIN), onTick, FALLING); // from high lo low
  resetToIdle(); //reset to idle
}
// LOOP
void loop() {
  sevseg.refreshDisplay();
  if (tickFlag) {
    noInterrupts();
    tickFlag = false; //tick is notices and marked as done
    unsigned long tickMsCopy = lastTickMs;
    interrupts();
    if (state==RUNNING) { //only react to the tick when game is on

      if (counterVal < TARGET) {
        counterVal++;
        showCount(counterVal);
        if (counterVal==TARGET) targetTickMs = tickMsCopy; // remember exactly when we reached 10 (target)
      } else {
        resultSuccess = false;
        state = RESULT; //change state from running to result
        showResult(false);
      }
    }
  }
  if (buttonPressedEvent()) {
    unsigned long nowMs = millis();
    if (state==IDLE) {
      counterVal = 0;
      showCount(0);
      state = RUNNING; // change state from idle to running (game has started, counter increaes, each tick will move the display)
    }
    else if (state==RUNNING) { //button pressed when the game was running
      unsigned long diff = 9999;
      if (counterVal==TARGET) diff = nowMs - targetTickMs; // nowMs the time button was pressed
      else if (counterVal==TARGET-1) { //if user pressed slighly early when the display is still at 9
        unsigned long expectedTarget = lastTickMs + 1000; // +1000 because one tick per second, so it is prediction of the target moment
        if (expectedTarget>nowMs) diff = expectedTarget-nowMs;
        else diff = nowMs-expectedTarget;
      }
      resultSuccess = (diff <= WIN_MS); //WIN_MS is 100 in our case
      state = RESULT;
      showResult(resultSuccess); // true shows good, false shows fail
    }
    else if (state==RESULT) resetToIdle();
  }
}
