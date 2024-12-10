// #include <Arduino.h>
#include <ESP32Servo.h>
 
Servo myservo;

int pos = 0;
// Recommended PWM GPIO pins on the ESP32 include 2,4,12-19,21-23,25-27,32-33 
 
void configServo(uint8_t servoPin) {
	// Allow allocation of all timers
	ESP32PWM::allocateTimer(0);
	ESP32PWM::allocateTimer(1);
	ESP32PWM::allocateTimer(2);
	ESP32PWM::allocateTimer(3);
	myservo.setPeriodHertz(50);    // standard 50 hz servo
	myservo.attach(servoPin, 500, 2800); // attaches the servo on pin 18 to the servo object
	// using default min/max of 1000us and 2000us
	// different servos may require different min/max settings
	// for an accurate 0 to 180 sweep
}
 
void rotateAround() {
    printf("\nhere we are..");
	for (pos = 0; pos <= 180; pos += 1) { // goes from 0 degrees to 180 degrees
		// in steps of 1 degree
		myservo.write(pos);    // tell servo to go to position in variable 'pos'
		delay(10);             // waits 15ms for the servo to reach the position
	}
        printf("\n2 loop!");
	for (pos = 180; pos >= 0; pos -= 1) { // goes from 180 degrees to 0 degrees
		myservo.write(pos);    // tell servo to go to position in variable 'pos'
		delay(10);             // waits 15ms for the servo to reach the position
	}
}

void reset(){
	myservo.write(0);
	delay(400);
	myservo.write(90);
	delay(400);
	myservo.write(180);
	delay(400);
	myservo.write(90);
	delay(400);
}
void rotate(uint8_t setdeg){
	myservo.write(setdeg);
}