  const int pinSW = 2;  
  const int pinX = A0;   
  const int pinY = A1;   
  const int ledUp = 6;
  const int ledDown = 9;
  const int ledLeft = 10;
  const int ledRight = 11;

  const int thresholdLow = 400; // Defining Thresholds & Dead-zone 
  const int thresholdHigh = 600; 

void setup() {

  Serial.begin(115200); // baud rate 115200 is used for faster sampling speed
  pinMode(pinSW, INPUT_PULLUP);
  pinMode(ledUp, OUTPUT);
  pinMode(ledDown, OUTPUT);
  pinMode(ledLeft, OUTPUT);
  pinMode(ledRight, OUTPUT);
}

void loop() {
  // Start timer for evidence of speed
  unsigned long startTime = micros();

  int valX = analogRead(pinX);
  int valY = analogRead(pinY);
  int valSW = digitalRead(pinSW); // Reads 0 if pressed, 1 if not

  digitalWrite(ledUp, LOW);
  digitalWrite(ledDown, LOW);
  digitalWrite(ledLeft, LOW);
  digitalWrite(ledRight, LOW);

  String direction = "CENTER";

  // DIRECTION LOGIC 
 
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

  // SWITCH LOGIC 
  // If valSW is LOW (0), the button is pressed
  if (valSW == LOW) {
    direction = direction + " + CLICK";
    digitalWrite(ledUp, HIGH);
    digitalWrite(ledDown, HIGH);
    digitalWrite(ledLeft, HIGH);
    digitalWrite(ledRight, HIGH);
  }

  // SERIAL OUTPUT
  Serial.print("Dir: ");
  Serial.print(direction);
  
  // Evidence of Sampling Speed 
  unsigned long endTime = micros();
  Serial.print(" | Loop Time (us): ");
  Serial.println(endTime - startTime);
  
  // A very short delay to keep the serial monitor readable
  delay(10); 
}
