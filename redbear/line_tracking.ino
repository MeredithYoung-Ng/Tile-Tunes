/*
 * Project line_tracking
 * Description: MOST RECENT VERSION
 * Author:
 * Date:
 */
Servo myservoL; //left servo
Servo myservoR; //right servo

const int zeroL = 1505;
const int zeroR = 1483;
const int speed = 15 ;

const int sensorPinL = A1;
const int buttonPin = A6;
const int sensorPinR = A0;
const int servoPinL = D16;
const int servoPinR = D4;
const int speedFull = 2300;
const int speedFwd = zeroL + speed; //left motor
const int speedBk = zeroR - speed; //right motor
const int speedStop = 1500;
const int speedFullBack = 700;


bool isOn = TRUE;
byte current_button = LOW;
byte old_button = LOW;
// setup() runs once, when the device is first turned on.
void setup() {
  // Put initialization like pinMode and begin functions here.
  Serial.begin(9600);
  myservoL.attach(servoPinL);
  myservoR.attach(servoPinR);
  //setup buttonPin
  pinMode(buttonPin, INPUT);
  attachInterrupt(sensorPinL, moveLef(), RISING);
  attachInterrupt(sensorPinR, moveRgt(), RISING);
}

void moveFwd(){
  //
  myservoR.writeMicroseconds(speedFwd);
  myservoL.writeMicroseconds(speedBk );
  delay(2);

    // myservoR.writeMicroseconds(speedFull);
    // myservoL.writeMicroseconds(speedFullBack);

}
void moveBck(){
  // TODO!!!
}
void moveLef(){
  myservoL.writeMicroseconds(speedBk -20);
  myservoR.writeMicroseconds(speedBk -10);
  // delay(100);
  // myservoR.writeMicroseconds(speedFwd);
  // analogWrite(myservoL);
  // analogWrite(myservoR);
}
void moveRgt(){
  myservoR.writeMicroseconds(speedFwd + 20);
  myservoL.writeMicroseconds(speedFwd + 10);

  // delay(10);
  // myservoL.writeMicroseconds(speedBk);
  // analogWrite(myservoL);
  // analogWrite(myservoR);
}
void stopAll(){
  myservoL.writeMicroseconds(speedStop);
  myservoR.writeMicroseconds(speedStop);
  // analogWrite(myservoL);
  // analogWrite(myservoR);
}

// loop() runs over and over again, as quickly as it can execute.
void loop() {
  int sensorL = digitalRead(sensorPinL);
  int sensorR = digitalRead(sensorPinR);
  Serial.print("Sensor L: ");
  Serial.println(sensorL);
  Serial.print("Sensor R: ");
  Serial.println(sensorR);

  // moveFwd();
  if ((sensorL==0)&&sensorR) {
        moveRgt();
        // moveLef();

      }
      else if ((sensorR==0)&&sensorL) {
        moveLef();

      }
      else {
        moveFwd();

      }
}
