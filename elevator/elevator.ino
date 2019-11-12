#include "TeensyStep.h"
#include "Shell.h"
#include "Bounce2.h"

//Stepper s1(34, 33);
StepControl controller(100);
RotateControl rc(100);

int32_t closed_position;

const uint32_t HOMING_SPEED = 500;
const uint32_t DOOR_SPEED = 6000;
const uint32_t DOOR_ACCEL = 1800;

class Door
{
  public:
  
  uint8_t pin_dir, pin_pulse, pin_sw1, pin_sw2;

  int motor_speed;
  int motor_accel;

  int32_t pos_open;
  int32_t pos_closed;

  Stepper s;
  
  
    Door(uint8_t pin_pulse, uint8_t pin_dir, uint8_t pin_sw1, uint8_t pin_sw2) 
      : s(pin_pulse, pin_dir)
    {
      this->pin_dir = pin_dir;
      this->pin_pulse = pin_pulse;
      this->pin_sw1 = pin_sw1;    
      this->pin_sw2 = pin_sw2;

      this->motor_speed = 400;
      this->motor_accel = 100;

//      pinMode(this->pin_dir, OUTPUT);
//      digitalWrite(this->pin_dir, HIGH);
//
//      pinMode(this->pin_pulse, OUTPUT);
//      digitalWrite(this->pin_pulse, HIGH);

      pinMode(this->pin_sw1, INPUT_PULLUP);
      pinMode(this->pin_sw2, INPUT_PULLUP);
    }

    void homing_cycle() {
      this->s.setAcceleration(1000);
      
      // open first
      if (!digitalRead(this->pin_sw1)) {
        this->s.setMaxSpeed(-HOMING_SPEED);
        rc.rotateAsync(this->s);
        while (!digitalRead(this->pin_sw1));
        rc.stop();
      }

      // reset the open position to 0
      this->s.setPosition(0);
      this->pos_closed = 18000;
      this->s.setMaxSpeed(HOMING_SPEED);
      
      if (!digitalRead(this->pin_sw2)) {
        this->s.setMaxSpeed(HOMING_SPEED);
        rc.rotateAsync(this->s);
        while (!digitalRead(this->pin_sw2));
        rc.stop();
      }

      // motor is now is now in the closed position
      this->pos_closed = this->s.getPosition();

      this->s.setMaxSpeed(DOOR_SPEED);
      this->s.setAcceleration(DOOR_ACCEL);
    }
};

//int32_t motor_speed = 4000;
//int32_t motor_accel = 1000;
//
//int32_t open_position = 0;
//int32_t closed_position;

Bounce button_start = Bounce();
Bounce button_stop = Bounce();

Door d1(34, 33, 35, 36);
Door d2(38, 37, 32, 39);

//void do_homing_cycle()
//{
//  s1.setAcceleration(1000);
//
//  if (!digitalRead(35)) {
//    s1.setMaxSpeed(-600);
//    rc.rotateAsync(s1);
//    while (!digitalRead(35));
//    rc.stop();
//  }
//
//  // reset the open position to 0
//  s1.setPosition(-10);
//
//  if (!digitalRead(36)) {
//    s1.setMaxSpeed(600);
//    rc.rotateAsync(s1);
//    while (!digitalRead(36));
//    rc.stop();
//  }
//
//  // motor is now is now in the closed position
//  closed_position = s1.getPosition();
//}

void setup() {
  // put your setup code here, to run once:
  //  pinMode(PIN_DIR, OUTPUT);
  //  pinMode(PIN_PULSE, OUTPUT);
//  pinMode(36, INPUT_PULLUP);
//  pinMode(35, INPUT_PULLUP);

//  button_start.attach(PIN_SW1, INPUT_PULLUP);
//  button_stop.attach(PIN_SW2, INPUT_PULLUP);

  Serial.begin(9600);
  shell_init(shell_reader, shell_writer, PSTR("Hi-Proof elevatormatic"));
  shell_register(command_home, PSTR("home"));
  shell_register(command_info, PSTR("info"));
  shell_register(command_open, PSTR("open"));
  shell_register(command_close, PSTR("close"));
  shell_register(command_move, PSTR("move"));

//  shell_register(command_motor, PSTR("motor"));
//  shell_register(command_motor, PSTR("m"));
  
  //
  //  s1.setTargetRel(-500);
  //  controller.move(s1);

  //shell_printf("Starting D1 homing cycle\r\n");
//    d1.homing_cycle();
//    d2.homing_cycle();
//  shell_printf("D1 closed position: %d\r\n", d1.pos_closed);

//  do_homing_cycle();
//  s1.setMaxSpeed(DOOR_SPEED);
//  s1.setAcceleration(DOOR_ACCEL);

  //command_home(0, NULL);
}



void loop() {
  //  Serial.printf("SW1: %d   SW2: %d\r\n", digitalRead(PIN_SW1), digitalRead(PIN_SW2));
  //  delay(500);

//  d1.s.setTargetAbs(0);
//  d2.s.setTargetAbs(0);
//  controller.move(d1.s, d2.s);
//  delay(1000);
//  d1.s.setTargetAbs(d1.pos_closed);
//  d2.s.setTargetAbs(d2.pos_closed);
//  controller.move(d1.s, d2.s);
//  delay(1000);

//  button_start.update();
//  button_stop.update();
//
//  if (button_start.rose()) {
//    rc.rotateAsync(s1);
//  }
//
//  if (button_stop.rose()) {
//    rc.emergencyStop();
//  }

  shell_task();
}

//-------------------------------------------------------------------------------------------
// CLI

int command_home(int argc, char** argv)
{
  shell_printf("Starting D1 homing cycle\r\n");
  d1.homing_cycle();
  shell_printf("D1 closed position: %d\r\n", d1.pos_closed);

  shell_printf("Starting D2 homing cycle\r\n");
  d2.homing_cycle();
  shell_printf("D1 closed position: %d\r\n", d2.pos_closed);
  
  return SHELL_RET_SUCCESS;
}

int command_info(int argc, char ** argv)
{
  shell_printf("D1 SW1: %d   SW2: %d   Pos: %d\r\n", digitalRead(d1.pin_sw1), digitalRead(d1.pin_sw2), d1.s.getPosition());
  shell_printf("D2 SW1: %d   SW2: %d   Pos: %d\r\n", digitalRead(d2.pin_sw1), digitalRead(d2.pin_sw2), d2.s.getPosition());
  return SHELL_RET_SUCCESS;
}

int command_open(int argc, char ** argv)
{
  d1.s.setTargetAbs(0);
  d2.s.setTargetAbs(0);
  controller.move(d1.s, d2.s);
  return SHELL_RET_SUCCESS;
}

int command_close(int argc, char ** argv)
{
  d1.s.setTargetAbs(d1.pos_closed);
  d2.s.setTargetAbs(d2.pos_closed);
  controller.move(d1.s, d2.s);
  return SHELL_RET_SUCCESS;
}

int command_move(int argc, char ** argv) {
  if (argc >= 2) {
    int new_pos = atoi(argv[1]);
    d1.s.setTargetRel(new_pos);
    controller.move(d1.s);
  }
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
