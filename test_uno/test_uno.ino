#include "Shell.h"

#define PULSE 6

int current_speed = 0;
int target_speed = 1000;
bool going = false;

void setup() {
  pinMode(PULSE, OUTPUT);

  Serial.begin(115200);
  shell_init(shell_reader, shell_writer, PSTR("Hi-Proof elevatormatic"));
  shell_register(command_speed, PSTR("s"));
  shell_register(command_go, PSTR("g"));
  shell_register(command_halt, PSTR("h"));
  shell_printf("Starting elevator software\r\n");

}

void loop() {
  shell_task();

  if (going) {
    if (current_speed > target_speed) {
      current_speed--; 
    }
    if (current_speed < target_speed) {
      current_speed++;
    }
    tone(PULSE, current_speed);
  } else {
    noTone(PULSE);
  }
  delay(1);
//  return; 
//  tone(PULSE, 1000);
//  delay(500);
//  tone(PULSE, 1500);
//  delay(500);
//  tone(PULSE, 2000);
//  delay(500);
//  tone(PULSE, 2500);
//  delay(500);
////  tone(PULSE, 3000);
////  delay(500);
////  tone(PULSE, 3500);
////  delay(500);
////  tone(PULSE, 4000);
////  delay(500);
////  tone(PULSE, 4500);
////  delay(500);
////  tone(PULSE, 5000);
////  delay(500);
////  tone(PULSE, 5500);
//  delay(50000);
}

//-------------------------------------------------------------------------------------------
// CLI

int command_go(int argc, char ** argv) 
{
  //tone(PULSE, current_speed); 
  going = true;
  return SHELL_RET_SUCCESS;
}

int command_halt(int argc, char ** argv) 
{
  going = false;
  //noTone(PULSE);
  return SHELL_RET_SUCCESS;
}

int command_speed(int argc, char ** argv) 
{
  if (argc > 1) {
    target_speed = atoi(argv[1]);
  }
  return SHELL_RET_SUCCESS;
}


//int command_motor(int argc, char** argv)
//{
//  if (argc == 1) {
//    shell_printf("Max speed:  %d\r\n", motor_speed);
//    shell_printf("Max accel:  %d\r\n", motor_accel);
//    shell_printf("Position:   %d\r\n", s1.getPosition());
//  }
//  if (argc >= 2) {
//    if (0 == strcmp(argv[1], "speed")) {
//      if (argc >= 3) {
//        motor_speed = atoi(argv[2]);
//        s1.setMaxSpeed(motor_speed);
//      }
//      shell_printf("Max speed:  %d\r\n", motor_speed);
//    }
//    if (0 == strcmp(argv[1], "accel")) {
//      if (argc >= 3) {
//        motor_accel = atoi(argv[2]);
//        s1.setAcceleration(motor_accel);
//      }
//      shell_printf("Max accel:  %d\r\n", motor_accel);      
//    }
//    if (0 == strcmp(argv[1], "start")) {
//      rc.rotateAsync(s1);
//    }
//    if (0 == strcmp(argv[1], "stop")) {
//      rc.stopAsync();
//    }
//    if (0 == strcmp(argv[1], "e")) {
//      rc.emergencyStop();
//    }
//
//  }
//  return SHELL_RET_SUCCESS;
//}

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
