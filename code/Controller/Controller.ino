/*
  Controller.ino
  License: Creative Commons 2013, Trevor Stanhope
*/

/*
  ERRORS
  0: No Error
  1: Object Too Close
  2: Object Too Far
  3: Load Failed
  4: Bad Action
  5. Blocked After Turning
  6. Right Orbit Failed
  7. Left Orbit Failed
  8. Right Avoid Failed
  9. Left Avoid Failed
*/

/* --- Headers --- */
#include "SoftwareSerial.h"
#include "stdio.h"
#include "Servo.h"
#include <AFMotor.h>
#define RIGHT_SERVO_PIN 10
#define LEFT_SERVO_PIN 9
#define LOAD_SERVO_PIN 11
#define ACTUATOR1_PWM_PIN 5 // Blue wire
#define ACTUATOR2_PWM_PIN 6 // Blue wire
#define ACTUATOR1_POSITION_PIN A4 // Purple wire
#define ACTUATOR2_POSITION_PIN A5 // Purple wire
#define ULTRASONIC_PIN 8
#define BAUD 9600
#define RANGE_GRAB 40 // maximum distance (cm) to grab object
#define RANGE_STUCK 10
#define TIME_WAIT 500
#define TIME_LEFT 100
#define TIME_RIGHT 100
#define TIME_FORWARD 2000
#define TIME_GRAB 3000
#define TIME_BACKWARD 2500
#define TIME_RACK 18000
#define TIME_RACK_END 4000
#define TIME_STEP 100
#define TIME_DEGREE 15
#define ACTUATOR_MAX 210
#define ACTUATOR_FULL 250
#define ACTUATOR_MIN 0
#define ACTUATOR_FEEDBACK_MAX 200
#define ACTUATOR_FEEDBACK_MIN 600
#define CR_SERVO_CCW 50
#define CR_SERVO_CW 150
#define CR_SERVO_STOP 100
#define S_SERVO_OPEN 0
#define S_SERVO_CLOSED 180
#define S_SERVO_CENTER 140

/* --- Prototypes --- */
char forward(void);
char backward(void);
char left(int val);
char right(int val);
char grab(void);
char dump(void);
char avoid_right(void);
char avoid_left(void);
char orbit_right(void);
char orbit_left(void);
long ping(void);
char wait(void);
char use_arm(int final);
char help_rack(void);
char use_rack(int final);

/* --- Error Codes --- */
const char ERROR_NONE = '0';
const char ERROR_ACTION = '1';
const char ERROR_CLOSE = '2';
const char ERROR_LOAD = '3';
const char ERROR_BLOCKED_RIGHT = '4';
const char ERROR_BLOCKED_LEFT = '5';
const char ERROR_ORBIT_RIGHT = '6';
const char ERROR_ORBIT_LEFT = '7';
const char ERROR_AVOID_RIGHT = '8';
const char ERROR_AVOID_LEFT = '9';

/* --- Basic Actions --- */
const char MOVE_FORWARD = 'F';
const char MOVE_BACKWARD = 'B';
const char NEAR_LEFT = 'K';
const char LEFT = 'L';
const char FAR_LEFT = 'M';
const char FULL_LEFT = 'N';
const char NEAR_RIGHT = 'Q';
const char RIGHT = 'R';
const char FAR_RIGHT = 'S';
const char FULL_RIGHT = 'T';
const char WAIT = 'W';
const char EXTEND_ARM = 'E';
const char USE_ARM = 'U';
const char CENTER_ARM = 'C';
const char TURN_AROUND = 'Z';
const char RESET_RACK = 'V';

/* --- Complex --- */
const char HELP_RACK = 'H';
const char GRAB = 'G';
const char DUMP = 'D';
const char AVOID_RIGHT = 'I';
const char AVOID_LEFT = 'J';
const char ORBIT_RIGHT = 'O';
const char ORBIT_LEFT = 'P';

/* --- Declarations ---*/
char action;
char error;
Servo left_servo;
Servo right_servo;
Servo load_servo;

/* --- Setup --- */
void setup() {
  left_servo.attach(LEFT_SERVO_PIN);
  right_servo.attach(RIGHT_SERVO_PIN);
  load_servo.attach(LOAD_SERVO_PIN);
  left_servo.write(CR_SERVO_STOP);
  right_servo.write(CR_SERVO_STOP);
  load_servo.write(S_SERVO_OPEN);
  Serial.begin(BAUD);
}

/* --- Loop --- */
void loop() {
  action = Serial.read();
  switch(action) {
    case HELP_RACK:
      error = help_rack();
      break;
    case MOVE_FORWARD:
      error = forward();
      break;
    case MOVE_BACKWARD:
      error = backward();
      break;
    case NEAR_LEFT:
      error = left(1);
      break;
    case LEFT:
      error = left(2);
      break;
    case FAR_LEFT:
      error = left(3);
      break;
    case FULL_LEFT:
      error = left(4);
      break;
    case NEAR_RIGHT:
      error = right(1);
      break;
    case RIGHT:
      error = right(2);
      break;
    case FAR_RIGHT:
      error = right(3);
      break;
    case FULL_RIGHT:
      error = right(4);
      break;
    case GRAB:
      error = grab();
      break;
    case DUMP:
      error = dump();
      break;
    case WAIT:
      error = wait();
      break;
    case ORBIT_RIGHT:
      error = orbit_right();
      break;
    case ORBIT_LEFT:
      error = orbit_left();
      break;
    case AVOID_RIGHT:
      error = avoid_right();
      break;
    case AVOID_LEFT:
      error = avoid_left();
      break;
    case EXTEND_ARM:
      error = use_arm(S_SERVO_OPEN);
      break;
    case USE_ARM:
      error = use_arm(S_SERVO_CLOSED);
      break;
    case CENTER_ARM:
      error = use_arm(S_SERVO_CENTER);
      break;
    case TURN_AROUND:
      error = right(25);
      break;
    case RESET_RACK:
      error = use_rack(0);
      break;
    default:
      error = ERROR_ACTION;
      break;
  }
  if (error != ERROR_ACTION) {
    Serial.flush();
    delay(TIME_WAIT);
    Serial.println(error);
  }
  else {
    Serial.flush();
  }
}

/* --- Forward --- */
// Attempt to move forward
char forward() {
  
  // Prepare
  char error;
  char temp;
  
  // Try
  right_servo.write(CR_SERVO_CW);
  left_servo.write(CR_SERVO_CCW);
  delay(TIME_FORWARD);
  right_servo.write(CR_SERVO_STOP);
  left_servo.write(CR_SERVO_STOP);

  // Finish
  if (ping() > RANGE_GRAB) {
    error = ERROR_NONE;
  }
  else {
    error = ERROR_CLOSE; // Object Too Close
  }
  return error;
}

/* --- Backward --- */
// Reverse backward
char backward() {
  
  // Prepare
  char error = ERROR_NONE;
  char temp;
  
  // Try
  right_servo.write(CR_SERVO_CCW);
  left_servo.write(CR_SERVO_CW);
  delay(TIME_BACKWARD);
  right_servo.write(CR_SERVO_STOP);
  left_servo.write(CR_SERVO_STOP);
  
  // Cleanup
  temp = use_arm(S_SERVO_CENTER);
  return error;
}

/* --- Left --- */
char left(int val) {
  
  // Prepare
  char error;
  char temp;
  
  // Do
  right_servo.write(CR_SERVO_CW);
  left_servo.write(CR_SERVO_CW);
  delay(val*TIME_LEFT);
  right_servo.write(CR_SERVO_STOP);
  left_servo.write(CR_SERVO_STOP);
  
  // Finish
  temp = use_arm(S_SERVO_CENTER);
  if (ping() < RANGE_GRAB) {
    error = ERROR_BLOCKED_LEFT;
  }
  else {
    error = ERROR_NONE;
  }
  return error;
}

/* --- Right --- */
char right(int val) {
  
  // Prepare
  char error;
  char temp;
    
  // Try
  right_servo.write(CR_SERVO_CCW);
  left_servo.write(CR_SERVO_CCW);
  delay(val*TIME_RIGHT);
  right_servo.write(CR_SERVO_STOP);
  left_servo.write(CR_SERVO_STOP);
  
  // Return Errors
  temp = use_arm(S_SERVO_CENTER);
  if (ping() < RANGE_GRAB) {
    error = ERROR_BLOCKED_RIGHT;
  }
  else {
    error = ERROR_NONE;
  }
  return error;  
}

/* --- Grab --- */
char grab() {
  
  // Prepare
  char error;
  char temp;

  // Try
  temp = use_arm(S_SERVO_OPEN);
  right_servo.write(CR_SERVO_CW);
  left_servo.write(CR_SERVO_CCW);
  delay(TIME_GRAB);
  right_servo.write(CR_SERVO_STOP);
  left_servo.write(CR_SERVO_STOP);
  temp = use_arm(S_SERVO_CLOSED);
  
  // Finish
  temp = use_arm(S_SERVO_CENTER);
  if (ping() < RANGE_GRAB) {
    error = ERROR_LOAD;
  }
  else {
    error = ERROR_NONE;
  }
  return error;
}
/* --- Help Rack --- */
char help_rack() {
  
  // Prepare
  char temp;
  
  // Try
  temp = use_arm(S_SERVO_CLOSED);
  analogWrite(ACTUATOR1_PWM_PIN, ACTUATOR_MAX);
  analogWrite(ACTUATOR2_PWM_PIN, ACTUATOR_MAX);             
  delay(TIME_RACK);
  analogWrite(ACTUATOR1_PWM_PIN, ACTUATOR_MIN);
  analogWrite(ACTUATOR2_PWM_PIN, ACTUATOR_MIN);             
  delay(TIME_RACK);
  
  // Finishe
  temp = use_arm(S_SERVO_CENTER);
  return ERROR_NONE;
}

/* --- Dump --- */
char dump() {
  
  // Prepare
  char error = ERROR_NONE;
  char temp;
  
  // Try
  temp = right(25);
  temp = use_arm(180);
  analogWrite(ACTUATOR1_PWM_PIN, ACTUATOR_MAX);
  analogWrite(ACTUATOR2_PWM_PIN, ACTUATOR_MAX);             
  delay(TIME_RACK);
  analogWrite(ACTUATOR1_PWM_PIN, ACTUATOR_FULL);
  analogWrite(ACTUATOR2_PWM_PIN, ACTUATOR_FULL);
  delay(TIME_RACK_END);
  temp = forward();
  analogWrite(ACTUATOR1_PWM_PIN, ACTUATOR_MIN);
  analogWrite(ACTUATOR2_PWM_PIN, ACTUATOR_MIN);             
  delay(TIME_RACK);
  
  // Finish
  temp = use_arm(S_SERVO_CENTER);
  return error;
}

/* --- Avoid Right --- */
char avoid_right() {
  
  // Prepare
  char error;
  char temp;
  
  // Try
  error = right(5);
  switch (error) {
    case ERROR_NONE:
      temp = forward();
      temp = left(7);
      temp = forward();
      break;
    case ERROR_BLOCKED_RIGHT:
      temp = left(5);
      error = ERROR_AVOID_RIGHT;
      break;
  }
  
  // Finish
  temp = use_arm(S_SERVO_CENTER);
  return error;
}

/* --- Left Avoid --- */
char avoid_left() {
  
  // Prepare
  char error;
  char temp;
  
  // Try
  error = left(5);
  switch (error) {
    case ERROR_NONE:
      temp = forward();
      temp = right(7);
      temp = forward();
      error = ERROR_NONE;
      break;
    case ERROR_BLOCKED_LEFT:
      temp = right(5);
      error = ERROR_AVOID_LEFT;
      break;
  }
  
  // Finish
  temp = use_arm(S_SERVO_CENTER);
  return error;
}

/* --- Right Orbit --- */
char orbit_right() {
  
  // Prepare
  char error;
  char temp;
  
  // Try
  temp = right(10);
  switch (temp) {
    case ERROR_NONE:
      temp = forward();
      temp = left(15);
      error = ERROR_NONE;
      break;
    default:
      temp = left(10);
      error = ERROR_ORBIT_RIGHT;
      break;
  }
  
  // Finish
  temp = use_arm(S_SERVO_CENTER);
  return error;
}

/* --- Left Orbit --- */
char orbit_left() {
  
  // Prepare
  char error;
  char temp;
  
  // Try
  temp = left(10);
  switch (temp) {
    case ERROR_NONE:
      temp = forward();
      temp = right(15);
      error = ERROR_NONE;
      break;
    default:
      temp = right(10);
      error = ERROR_ORBIT_LEFT;
      break;
  }
  
  // Finish
  temp = use_arm(S_SERVO_CENTER);
  return error;
}

/* --- Wait --- */
char wait() {
  
  // Prepare
  char error = ERROR_NONE;
  char temp;
  
  // Try
  delay(TIME_WAIT);
  
  // Finish
  temp = use_arm(S_SERVO_CENTER);
  return error;
}

/* --- Ping --- */
// Ping for distance to closest object
long ping() {
  
  // Prepare
  long centimeters;
  
  // Try
  pinMode(ULTRASONIC_PIN, OUTPUT);
  digitalWrite(ULTRASONIC_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(ULTRASONIC_PIN, HIGH);
  delayMicroseconds(5);
  digitalWrite(ULTRASONIC_PIN, LOW);
  pinMode(ULTRASONIC_PIN, INPUT);
  centimeters = pulseIn(ULTRASONIC_PIN, HIGH) / 28 / 2;
  
  // Finish
  return centimeters;
}

/* --- Use Arm --- */
char use_arm(int final) {
  int start = load_servo.read();
  if (start > final) {
    for (int degree = start; degree > final; degree--) {
      load_servo.write(degree);
      delay(TIME_DEGREE);
    }
  }
  else {
    for (int degree = start; degree < final; degree++) {
      load_servo.write(degree);
      delay(TIME_DEGREE);
    }
  }
  return ERROR_NONE;
}

/* --- Lower Rack --- */
char use_rack(int final) {
  
  // Prepare
  char error;
  char temp;

  // Try
  temp = use_arm(S_SERVO_CLOSED);
  analogWrite(ACTUATOR1_PWM_PIN, final);
  analogWrite(ACTUATOR2_PWM_PIN, final);
  delay(final*100);
  
  // Finish
  return ERROR_NONE;
}
