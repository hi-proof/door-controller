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
}

//-------------------------------------------------------------------------------------------
// shell commands

int command_go(int argc, char ** argv) 
{
  going = true;
  return SHELL_RET_SUCCESS;
}

int command_halt(int argc, char ** argv) 
{
  going = false;
  return SHELL_RET_SUCCESS;
}

int command_speed(int argc, char ** argv) 
{
  if (argc > 1) {
    target_speed = atoi(argv[1]);
  }
  return SHELL_RET_SUCCESS;
}

//-------------------------------------------------------------------------------------------
// shell boilerplate to work with serial

int shell_reader(char * data)
{
  if (Serial.available()) {
    *data = Serial.read();
    return 1;
  }
  return 0;
}

void shell_writer(char data)
{
  Serial.write(data);
}
