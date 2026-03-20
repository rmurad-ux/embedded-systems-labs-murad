#include <LiquidCrystal.h>
#include <Math.h>

LiquidCrystal lcd(12, 11, 5, 4, 3, 6); // LCD: RS, E, D4, D5, D6, D7

const byte LED_PIN = 8; 
const byte MIC_AO  = A0;
const byte MIC_DO  = 2;  

int soundLevel = 0;   // processed sound level, soundlevel is computed using SignalMax - SignalMin
int THRESHOLD  = 10;    
bool aboveThreshold = false; //bool stores T/F and it stores current state, starting with false, assuming the sound is not above the threshold
bool wasAbove = false; // stores previous state 
unsigned long eventCount = 0; // how many sound events have been detected, 

float dBLevel = 0.0;

// sampling window
unsigned long lastWindowStart = 0; //when window started, so here it start at 0 ms
const unsigned long WINDOW_MS = 50; // window lenght arduino collects microphone samples for 50 millisecond(it remembers the signalMin and signalMax and then computes soundlevel)
int signalMin = 1023;
int signalMax = 0; // to find the max you should start from the smallest possible value, if it was signalMax = 1023, then there is no value bigger than that, so max should be 1023

// LED non-blocking
unsigned long ledStartTime = 0; // stores the moment when the LED was turned on 
bool ledOn = false; // false at start because the led should begin off
const unsigned long LED_ON_TIME = 300; // how long led should stay on after the detection event 

// LCD / Serial timing
unsigned long lastLcdUpdate = 0; // checks when LCD was updated last time 
const unsigned long LCD_INTERVAL = 100; //update LCD every 100ms, not every loop

unsigned long lastSerialUpdate = 0;
const unsigned long SERIAL_INTERVAL = 100; // update serial every 100ms

void setup() {
  pinMode(LED_PIN, OUTPUT); //output as arduino sends voltage to that pin
  digitalWrite(LED_PIN, LOW); //led starts off

  pinMode(MIC_DO, INPUT); // optional, for MIC_A0 we dont use pinMode as analog pins are used with analogRead. input because microphone sends signal to arduino

  lcd.begin(16, 2); // initiliaze lcd, 16 columns 2 rows
  lcd.clear(); // clears the lcd screen 
  lcd.setCursor(0, 0); // move cursor to column 0 and row 0
  lcd.print("Sound Monitor");
  lcd.setCursor(0, 1);
  lcd.print("Init...");

  Serial.begin(115200);
  delay(800); // waits for 800ms and clears the sound monitor and init...
  lcd.clear();

  lastWindowStart = millis();
}
void loop() {
  unsigned long now = millis(); //gets the arduino time in milliseconds

  // 1) continuously sample analog microphone signal
  int raw = analogRead(MIC_AO);

  if (raw < signalMin) signalMin = raw; // signalmin (1023) and signalmax (0) from values in previous line
  if (raw > signalMax) signalMax = raw;

  // 2) every WINDOW_MS, compute sound level
  if (now - lastWindowStart >= WINDOW_MS) { // it asks has 50ms passed since the window started? 
    soundLevel = signalMax - signalMin;

        if (soundLevel < 1) { // log10(0) is undefined
      dBLevel = 0.0;
    } else {
      dBLevel = 20.0 * log10((float)soundLevel); //A ref is 1
    }

    aboveThreshold = (soundLevel >= THRESHOLD);

    // rising-edge detection
    if (aboveThreshold && !wasAbove) { // sound is loud now and was not loud before (!)
      eventCount++;
      digitalWrite(LED_PIN, HIGH);
      ledOn = true;
      ledStartTime = now; // need this data for further calculations to turn off the led
    }
    wasAbove = aboveThreshold; // if sound is still loud meaning both true=true, then no event is counted

    // reset for next window
    signalMin = 1023;
    signalMax = 0;
    lastWindowStart = now;
  }
if (ledOn && (now - ledStartTime >= LED_ON_TIME)) { // if led is on and condition has passed 300 ms then turn led off
    digitalWrite(LED_PIN, LOW);
    ledOn = false; //led is no longer on, so it resets 
  }
// 4) LCD update
  if (now - lastLcdUpdate >= LCD_INTERVAL) { // updating the LCD if enough time has passed (LCD_INTERVAL = 100 ms))
    lastLcdUpdate = now;

    lcd.setCursor(0, 0); // first row
    lcd.print("                ");   // clear row
    lcd.setCursor(0, 0);
    lcd.print("L:");
    lcd.print(soundLevel);
    lcd.print(" ");
    lcd.print((int)dBLevel);
    lcd.print("dB");

    lcd.setCursor(0, 1); // second row 
    lcd.print("                ");   // clear row
    lcd.setCursor(0, 1);
    lcd.print("T:");
    lcd.print(THRESHOLD);
    lcd.print(" ");

    if (aboveThreshold) {
      lcd.print("LOUD");
    } else {
      lcd.print("QUIET");
    }
  }

  // 5) Serial output
  if (now - lastSerialUpdate >= SERIAL_INTERVAL) {  // new serial data is sent only if 100 ms has passed from previous update
    lastSerialUpdate = now; //now is stored in millis, as a result, lastSerialUpdate is shown in millis

    Serial.print("Level=");
    Serial.print(soundLevel);
    Serial.print(", dB=");
    Serial.print(dBLevel);
    Serial.print(", Threshold=");
    Serial.print(THRESHOLD);
    Serial.print(", Above=");
    Serial.print(aboveThreshold ? 1 : 0); // if abovethreshold is true = 1, false print 0
    Serial.print(", Events=");
    Serial.println(eventCount);
  }
}
