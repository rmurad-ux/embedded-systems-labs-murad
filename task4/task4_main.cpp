#include <Arduino.h>

const int pinSW = 2;   
const int pinX = A0;   
const int pinY = A1;   

const int ledUp = 6;
const int ledDown = 9;
const int ledLeft = 10;
const int ledRight = 11;
const int thresholdLow = 400;  
const int thresholdHigh = 600; 

void setup() {
  Serial.begin(115200); 
  pinMode(pinSW, INPUT_PULLUP);
  pinMode(ledUp, OUTPUT);
  pinMode(ledDown, OUTPUT);
  pinMode(ledLeft, OUTPUT);
  pinMode(ledRight, OUTPUT);
}

void loop() {
  unsigned long startTime = micros();

  int valX = analogRead(pinX); // read inputs
  int valY = analogRead(pinY);
  int valSW = digitalRead(pinSW); // Reads 0 if pressed, 1 if not

  digitalWrite(ledUp, LOW);
  digitalWrite(ledDown, LOW);
  digitalWrite(ledLeft, LOW);
  digitalWrite(ledRight, LOW);

  String direction = "CENTER";

  if (valY < thresholdLow) {
    digitalWrite(ledUp, HIGH);
    direction = "UP";
  } 
  else if (valY > thresholdHigh) {
    digitalWrite(ledDown, HIGH);
    direction = "DOWN";
  }

  if (valX < thresholdLow) {
    digitalWrite(ledLeft, HIGH);
    direction = "LEFT";
  } 
  else if (valX > thresholdHigh) {
    digitalWrite(ledRight, HIGH);
    direction = "RIGHT";
  }


  
  if (valSW == LOW) {       //LOW (0), the button is pressed
    direction = direction + " + CLICK";
    digitalWrite(ledUp, HIGH);
    digitalWrite(ledDown, HIGH);
    digitalWrite(ledLeft, HIGH);
    digitalWrite(ledRight, HIGH);
  }

  Serial.print(valX);
  Serial.print(",");
  Serial.print(valY);
  Serial.print(",");
  Serial.print(valSW);
  Serial.print(",");
  
  unsigned long endTime = micros();
  unsigned long loopTime = endTime - startTime;
  Serial.println(loopTime);
  
  delay(10); 
}
