/*********************************************************************
    ROSArduinoBridge

    A set of simple serial commands to control a differential drive
    robot and receive back sensor and odometry data. Default
    configuration assumes use of an Arduino Mega + Pololu motor
    controller shield + Robogaia Mega Encoder shield.  Edit the
    readEncoder() and setMotorSpeed() wrapper functions if using
    different motor controller or encoder method.

    Created for the Pi Robot Project: http://www.pirobot.org
    and the Home Brew Robotics Club (HBRC): http://hbrobotics.org

    Authors: Patrick Goebel, James Nugen

    Inspired and modeled after the ArbotiX driver by Michael Ferguson

    Software License Agreement (BSD License)

    Copyright (c) 2012, Patrick Goebel.
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:

       Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
       Redistributions in binary form must reproduce the above
       copyright notice, this list of conditions and the following
       disclaimer in the documentation and/or other materials provided
       with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
    FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
    COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
    BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
    LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
    ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGE.
 *********************************************************************/

#define USE_BASE      // Enable the base controller code
//#undef USE_BASE     // Disable the base controller code

/* Define the motor controller and encoder library you are using */
#ifdef USE_BASE
/* The Pololu VNH5019 dual motor driver shield */
//#define POLOLU_VNH5019

/* The L298P dual motor driver shield */
/* Default PinMode DIR1,DIR2,PWM1,PWM2 ;sunMaxwell at 2017.12.10*/
#define L298P
/* Enable TB6612FNG PinMode IN1,IN2,IN3,IN4,ENA,ENB ;sunMaxwell at 2017.12.10*/
#define TB6612FNG
//#define L298P_4WD

/* The Pololu MC33926 dual motor driver shield */
//#define POLOLU_MC33926

/* The RoboGaia encoder shield */
//#define ROBOGAIA

/* Encoders directly attached to Arduino board */
#define ARDUINO_ENC_COUNTER
#endif

#define USE_SERVOS  // Enable use of PWM servos as defined in servos.h
//#undef USE_SERVOS     // Disable use of PWM servos

/* Serial port baud rate */
#define BAUDRATE     57600

/* Maximum PWM signal */
#define MAX_PWM        255

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

/* Include definition of serial commands */
#include "commands.h"

/* Sensor functions */
#include "sensors.h"

/* Include servo support if required */
#ifdef USE_SERVOS
//#include <Servo.h>
#include "servos.h"
#include <Wire.h>
#include <ServoDriver.h>
#define SERVOMIN  102 // this is the 'minimum' pulse length count (out of 4096)
#define SERVOMAX  512 // this is the 'maximum' pulse length count (out of 4096)
ServoDriver servodriver = ServoDriver();
#endif

#ifdef USE_BASE
/* Motor driver function definitions */
#include "motor_driver.h"

/* Encoder driver function definitions */
#include "encoder_driver.h"

/* PID parameters and functions */
#include "diff_controller.h"

/* Run the PID loop at 30 times per second */
#define PID_RATE           30     // Hz

/* Convert the rate into an interval */
const int PID_INTERVAL = 1000 / PID_RATE;

/* Track the next time we make a PID calculation */
unsigned long nextPID = PID_INTERVAL;

/* Stop the robot if it hasn't received a movement command
  in this number of milliseconds */
#define AUTO_STOP_INTERVAL 2000
long lastMotorCommand = AUTO_STOP_INTERVAL;
#endif

/* Variable initialization */

// A pair of varibles to help parse serial commands (thanks Fergs)
int arg = 0;
int index = 0;

// Variable to hold an input character
char chr;

// Variable to hold the current single-character command
char cmd;

// Character arrays to hold the first and second arguments
char argv1[64];
char argv2[64];

// The arguments converted to integers
long arg1;
long arg2;

int test=0;

/* Clear the current command parameters */
void resetCommand() {
  cmd = NULL;
  memset(argv1, 0, sizeof(argv1));
  memset(argv2, 0, sizeof(argv2));
  arg1 = 0;
  arg2 = 0;
  arg = 0;
  index = 0;
}

/* Run a command.  Commands are defined in commands.h */
int runCommand() {
  int i = 0;
  char *p = argv1;
  char *str;
  int pid_args[8];
  arg1 = atoi(argv1);
  arg2 = atoi(argv2);

//  test

//  if (test==0){
//    moving=1;
//    arg1=40;
//    arg2=40;
//    test=1;
//    cmd=MOTOR_SPEEDS;
//  }else{
//    cmd=READ_ENCODERS;
//  }
// Serial.println("runCommand--------");
  ///
  switch (cmd) {
    case GET_BAUDRATE:
      Serial.println(BAUDRATE);
      break;
    case READ_PIDIN:
      Serial.print(readPidIn(LEFT));
      Serial.print(" ");
#ifdef L298P      
      Serial.println(readPidIn(RIGHT));
#endif
#ifdef L298P_4WD 
      Serial.print(readPidIn(RIGHT));
      Serial.print(" ");
      Serial.print(readPidIn(LEFT_H));
      Serial.print(" ");
      Serial.println(readPidIn(RIGHT_H));
#endif      
      break;
    case READ_PIDOUT:
      Serial.print(readPidOut(LEFT));
      Serial.print(" ");
#ifdef L298P      
      Serial.println(readPidOut(RIGHT));
#endif
#ifdef L298P_4WD 
      Serial.print(readPidOut(RIGHT));
      Serial.print(" ");
      Serial.print(readPidOut(LEFT_H));
      Serial.print(" ");
      Serial.println(readPidOut(RIGHT_H));
#endif       
      break;
    case ANALOG_READ:
      Serial.println(analogRead(arg1));
      break;
    case DIGITAL_READ:
      Serial.println(digitalRead(arg1));
      break;
    case ANALOG_WRITE:
      analogWrite(arg1, arg2);
      Serial.println("OK");
      break;
    case DIGITAL_WRITE:
      if (arg2 == 0) digitalWrite(arg1, LOW);
      else if (arg2 == 1) digitalWrite(arg1, HIGH);
      Serial.println("OK");
      break;
    case PIN_MODE:
      if (arg2 == 0) pinMode(arg1, INPUT);
      else if (arg2 == 1) pinMode(arg1, OUTPUT);
      Serial.println("OK");
      break;
    case PING:
      Serial.println(Ping(arg1));
      break;
#ifdef USE_SERVOS
    case SERVO_WRITE:
      //servos[arg1].write(arg2);
      servodriver.setPWM(arg1,0,int(SERVOMIN+arg2*(SERVOMAX-SERVOMIN)/180));
      servosPos[arg1]=int(arg2);//short(SERVOMIN+arg2*(SERVOMAX-SERVOMIN)/180);
      Serial.println("OK");
      break;
    case SERVO_READ:
      //Serial.println(servos[arg1].read());
      Serial.println(servosPos[arg1]);
      break;
#endif

#ifdef USE_BASE
    case READ_ENCODERS:
      Serial.print(readEncoder(LEFT));
      Serial.print(" ");
#ifdef L298P 
      Serial.println(readEncoder(RIGHT));
#endif
#ifdef L298P_4WD
      Serial.print(readEncoder(RIGHT));
      Serial.print(" ");
      Serial.print(readEncoder(LEFT_H));
      Serial.print(" ");
      Serial.println(readEncoder(RIGHT_H));
#endif      
      break;
    case RESET_ENCODERS:
      resetEncoders();
      resetPID();
      Serial.println("OK");
      break;
    case MOTOR_SPEEDS:
      /* Reset the auto stop timer */
      lastMotorCommand = millis();
      if (arg1 == 0 && arg2 == 0) {
#ifdef L298P        
        setMotorSpeeds(0, 0);
#endif

#ifdef L298P_4WD
        setMotorSpeeds(0, 0, 0, 0);
#endif        
        moving = 0;
      }
      else moving = 1;
      leftPID.TargetTicksPerFrame = arg1;
      rightPID.TargetTicksPerFrame = arg2;
#ifdef L298P_4WD      
      leftPID_h.TargetTicksPerFrame = arg1;
      rightPID_h.TargetTicksPerFrame = arg2;
#endif       
      Serial.println("OK");
      break;
    case UPDATE_PID:
      while ((str = strtok_r(p, ":", &p)) != '\0') {
        pid_args[i] = atoi(str);
        i++;
      }
//      Kp = pid_args[0];
//      Kd = pid_args[1];
//      Ki = pid_args[2];
//      Ko = pid_args[3];

      left_Kp = pid_args[0];
      left_Kd = pid_args[1];
      left_Ki = pid_args[2];
      left_Ko = pid_args[3];

      right_Kp = pid_args[4];
      right_Kd = pid_args[5];
      right_Ki = pid_args[6];
      right_Ko = pid_args[7];

#ifdef L298P_4WD

      left_h_Kp = pid_args[0];
      left_h_Kd = pid_args[1];
      left_h_Ki = pid_args[2];
      left_h_Ko = pid_args[3];

      right_h_Kp = pid_args[4];
      right_h_Kd = pid_args[5];
      right_h_Ki = pid_args[6];
      right_h_Ko = pid_args[7];
#endif
      
      Serial.println("OK");
      break;
#endif
    default:
      Serial.println("Invalid Command");
      break;
  }
}

/* Setup function--runs once at startup. */
void setup() {
  Serial.begin(BAUDRATE);

  // Initialize the motor controller if used */
#ifdef USE_BASE
#ifdef ARDUINO_ENC_COUNTER
  //set as inputs
  DDRD &= ~(1 << LEFT_ENC_PIN_A);
  DDRD &= ~(1 << LEFT_ENC_PIN_B);
  DDRC &= ~(1 << RIGHT_ENC_PIN_A);
  DDRC &= ~(1 << RIGHT_ENC_PIN_B);

#ifdef L298P_4WD
  DDRD &= ~(1 << LEFT_H_ENC_PIN_A);
  DDRD &= ~(1 << LEFT_H_ENC_PIN_B);
  DDRC &= ~(1 << RIGHT_H_ENC_PIN_A);
  DDRC &= ~(1 << RIGHT_H_ENC_PIN_B);
#endif  

  //enable pull up resistors
  PORTD |= (1 << LEFT_ENC_PIN_A);
  PORTD |= (1 << LEFT_ENC_PIN_B);
  PORTC |= (1 << RIGHT_ENC_PIN_A);
  PORTC |= (1 << RIGHT_ENC_PIN_B);

#ifdef L298P_4WD
  PORTD &= ~(1 << LEFT_H_ENC_PIN_A);
  PORTD &= ~(1 << LEFT_H_ENC_PIN_B);
  PORTC &= ~(1 << RIGHT_H_ENC_PIN_A);
  PORTC &= ~(1 << RIGHT_H_ENC_PIN_B);
#endif    
  // tell pin change mask to listen to left encoder pins
  PCMSK2 |= (1 << LEFT_ENC_PIN_A) | (1 << LEFT_ENC_PIN_B);
  // tell pin change mask to listen to right encoder pins
  PCMSK1 |= (1 << RIGHT_ENC_PIN_A) | (1 << RIGHT_ENC_PIN_B);

#ifdef L298P_4WD
  // tell pin change mask to listen to left encoder pins
  PCMSK2 |= (1 << LEFT_ENC_PIN_A) | (1 << LEFT_ENC_PIN_B) | (1 << LEFT_H_ENC_PIN_A) | (1 << LEFT_H_ENC_PIN_B);
  // tell pin change mask to listen to right encoder pins
  PCMSK1 |= (1 << RIGHT_ENC_PIN_A) | (1 << RIGHT_ENC_PIN_B) | (1 << RIGHT_H_ENC_PIN_A) | (1 << RIGHT_H_ENC_PIN_B);
#endif
  // enable PCINT1 and PCINT2 interrupt in the general interrupt mask
  PCICR |= (1 << PCIE1) | (1 << PCIE2);
#endif
  initMotorController();
  resetPID();
#endif

  /* Attach servos if used */
#ifdef USE_SERVOS
  //int i;
  //for (i = 0; i < N_SERVOS; i++) {
    //servos[i].attach(servoPins[i]);
  //}
  int i;
  for (i = 0; i < N_SERVOS; i++) {
    servosPos[i]=90;
  }
  servodriver.begin();
  servodriver.setPWMFreq(50);
#endif
}

/* Enter the main loop.  Read and parse input from the serial port
   and run any valid commands. Run a PID calculation at the target
   interval and check for auto-stop conditions.
*/
void loop() {
  //setMotorSpeeds(128, 128);
  /*moving = 1;
  Serial.println("---------------------------");
  Serial.println(readEncoder(LEFT));
  Serial.println(readEncoder(RIGHT));
  Serial.println("************");
  Serial.println(readPidIn(LEFT));
  Serial.println(readPidIn(RIGHT));
  Serial.println("************");
  Serial.println(readPidOut(LEFT));
  Serial.println(readPidOut(RIGHT));
  
  leftPID.TargetTicksPerFrame=12;
  rightPID.TargetTicksPerFrame = 12;
  if (millis() > nextPID) {
    updatePID();
    nextPID += PID_INTERVAL;
  }*/
  //runCommand();
  //u 10:12:0:50:10:12:0:50
  while (Serial.available() > 0) {

    // Read the next character
    chr = Serial.read();

    // Terminate a command with a CR
    if (chr == 13) {
      if (arg == 1) argv1[index] = NULL;
      else if (arg == 2) argv2[index] = NULL;
      runCommand();
      resetCommand();
    }
    // Use spaces to delimit parts of the command
    else if (chr == ' ') {
      // Step through the arguments
      if (arg == 0) arg = 1;
      else if (arg == 1)  {
        argv1[index] = NULL;
        arg = 2;
        index = 0;
      }
      continue;
    }
    else {
      if (arg == 0) {
        // The first arg is the single-letter command
        cmd = chr;
      }
      else if (arg == 1) {
        // Subsequent arguments can be more than one character
        argv1[index] = chr;
        index++;
      }
      else if (arg == 2) {
        argv2[index] = chr;
        index++;
      }
    }
  }
  //runCommand();
  // If we are using base control, run a PID calculation at the appropriate intervals
#ifdef USE_BASE
  if (millis() > nextPID) {
    updatePID();
    nextPID += PID_INTERVAL;
  }

  // Check to see if we have exceeded the auto-stop interval
  if ((millis() - lastMotorCommand) > AUTO_STOP_INTERVAL) {
    ;
#ifdef L298P    
    setMotorSpeeds(0, 0);
#endif
#ifdef L298P_4WD
    setMotorSpeeds(0, 0, 0, 0);
#endif
    moving = 0;
  }

#endif
}






