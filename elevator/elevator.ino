#include "TeensyStep.h"
#include "Shell.h"

//l2 green dir 8
//l4 red pulse 20

#define PIN_DIR 8
#define PIN_PULSE 20

#define PIN_SW1  15
#define PIN_SW2  39

StepControl controller;
RotateControl rc;
Stepper s1(PIN_PULSE, PIN_DIR);

int32_t motor_speed = 100;
int32_t motor_accel = 100;

int32_t open_position = 0;
int32_t closed_position;

void do_homing_cycle()
{
  s1.setAcceleration(1000);

//  RotateControl rc;

  if (!digitalRead(PIN_SW1)) {
    s1.setMaxSpeed(-150);
    rc.rotateAsync(s1);
    while (!digitalRead(PIN_SW1));
    rc.stop();
  }

  // reset the open position to 0
  s1.setPosition(-10);

  if (!digitalRead(PIN_SW2)) {
    s1.setMaxSpeed(150);
    rc.rotateAsync(s1);
    while (!digitalRead(PIN_SW2));
    rc.stop();
  }

  // motor is now is now in the closed position
  closed_position = s1.getPosition();
}


void setup() {
  // put your setup code here, to run once:
  //  pinMode(PIN_DIR, OUTPUT);
  //  pinMode(PIN_PULSE, OUTPUT);
  pinMode(PIN_SW1, INPUT_PULLUP);
  pinMode(PIN_SW2, INPUT_PULLUP);

  Serial.begin(9600);
  shell_init(shell_reader, shell_writer, PSTR("Hi-Proof elevatormatic"));
  shell_register(command_motor, PSTR("motor"));
  shell_register(command_motor, PSTR("m"));
  shell_printf("Starting elevator software\r\n");
  
  //
  //  s1.setTargetRel(-500);
  //  controller.move(s1);

//  do_homing_cycle();
//  s1.setMaxSpeed(350);
//  s1.setAcceleration(200);
}



void loop() {
  //  Serial.printf("SW1: %d   SW2: %d\r\n", digitalRead(PIN_SW1), digitalRead(PIN_SW2));
  //  delay(500);

//  s1.setTargetAbs(open_position);
//  controller.move(s1);
//  delay(1000);
//  s1.setTargetAbs(closed_position);
//  controller.move(s1);
//  delay(1000);

  shell_task();
}

//-------------------------------------------------------------------------------------------
// CLI

int command_motor(int argc, char** argv)
{
  if (argc == 1) {
    shell_printf("Max speed:  %d\r\n", motor_speed);
    shell_printf("Max accel:  %d\r\n", motor_accel);
    shell_printf("Position:   %d\r\n", s1.getPosition());
  }
  if (argc >= 2) {
    if (0 == strcmp(argv[1], "speed")) {
      if (argc >= 3) {
        motor_speed = atoi(argv[2]);
        s1.setMaxSpeed(motor_speed);
      }
      shell_printf("Max speed:  %d\r\n", motor_speed);
    }
    if (0 == strcmp(argv[1], "accel")) {
      if (argc >= 3) {
        motor_accel = atoi(argv[2]);
        s1.setAcceleration(motor_accel);
      }
      shell_printf("Max accel:  %d\r\n", motor_accel);      
    }
    if (0 == strcmp(argv[1], "start")) {
      rc.rotateAsync(s1);
    }
    if (0 == strcmp(argv[1], "stop")) {
      rc.stopAsync();
    }
    if (0 == strcmp(argv[1], "e")) {
      rc.emergencyStop();
    }

  }
  return SHELL_RET_SUCCESS;
}

int shell_reader(char * data)
{
  // Wrapper for Serial.read() method
  if (Serial.available()) {
    *data = Serial.read();
    return 1;
  }
  return 0;
}

void shell_writer(char data)
{
  // Wrapper for Serial.write() method
  Serial.write(data);
}
