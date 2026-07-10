#include <Servo.h>

// assign variable "SERVO_PIN" to GPIO 26
#define SERVO_PIN 26
#define SERVO_RANGE 180

// create Servo object
Servo myServo;

void setup() {
  // start Serial Monitor
  Serial.begin(9600);
  // define GPIO 26 ("SERVO_PIN") as signal pin of servo motor
  myServo.attach(SERVO_PIN);
}

void loop() {
  // drive servo to 0 degree position
  myServo.write(0);
  Serial.println("0");
  delay(500);

  // drive servo to 45 degree position
  myServo.write(SERVO_RANGE/4);
  Serial.println(SERVO_RANGE/4);
  delay(500);

  // drive servo to 90 degree position
  myServo.write(SERVO_RANGE/2);
  Serial.println(SERVO_RANGE/2);
  delay(500);

  // drive servo to 135 degree position
  myServo.write(SERVO_RANGE/4*3);
  Serial.println(SERVO_RANGE/4*3);
  delay(500);

  // drive servo to 180 degree position
  myServo.write(SERVO_RANGE);
  Serial.println(SERVO_RANGE);
  delay(1000);
}