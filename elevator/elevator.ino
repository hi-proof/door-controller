#include "TeensyStep.h"
#include "Shell.h"
#include "Bounce2.h"
#include "parallelio.h"

#include "floor_controller.h"
#include "call_panel.h"

namespace {
  // Device states

  enum class DeviceStates : uint8_t {
    Normal,
    Uncalibrated,
    FloorProgramming,
    DoorProgramming,
    Offline,
  };
  DeviceStates current_state{DeviceStates::Uncalibrated};

} // anonymous namespace

//Stepper s1(34, 33);
StepControl controller(25);
RotateControl rc(25);

hiproof::elevator::FloorController floor_ctrl{34, 33, 35, 36};
hiproof::elevator::CallPanel call_panel{};

int32_t closed_position;

const uint32_t HOMING_SPEED = 1000;
const uint32_t HOMING_ACCEL = 1000;
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

      this->motor_speed = 1500;
      this->motor_accel = 500;

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

    void homing_cycle_rotate() {
      this->s.setAcceleration(HOMING_ACCEL);

      // open door until both switches are triggered
      if (!digitalRead(this->pin_sw1) && !digitalRead(this->pin_sw2)) {
        this->s.setMaxSpeed(-HOMING_SPEED);
        rc.rotateAsync(this->s);

        // wait for the second switch to be triggered
        while (true) {
          while (!digitalRead(this->pin_sw1));
          // if both switches are triggered we're at the base position
          if (digitalRead(this->pin_sw2)) {
             break;
          }
        }
        rc.stop();
      }

      // we're back home - reset position
      this->s.setPosition(0);
    }

    void goto_position(int32_t pos) {
      s.setMaxSpeed(motor_speed);
      s.setAcceleration(motor_accel);
      s.setTargetAbs(pos);
      controller.moveAsync(s);
    }

    void go()
    {
       rc.stop();
       s.setMaxSpeed(HOMING_SPEED);
       s.setAcceleration(HOMING_ACCEL);
       rc.rotateAsync(s);
    }

    void back()
    {
       rc.stop();
       s.setMaxSpeed(-HOMING_SPEED);
       s.setAcceleration(HOMING_ACCEL);
       rc.rotateAsync(s);
    }
    
    void halt()
    {
       rc.stopAsync();
    }
    
    void estop()
    {
       controller.emergencyStop();
       rc.emergencyStop();
    }
};


int32_t motor_speed = 400;
int32_t motor_accel = 500;
//
//int32_t open_position = 0;
//int32_t closed_position;

Bounce button_start = Bounce();
Bounce button_stop = Bounce();

// Door d1(34, 33, 35, 36);
Door d2(38, 37, 32, 39);


#define SW_DATA (26)
#define SW_CLOCK (27)
#define SW_LATCH (25)
ParallelInputs buttons(SW_DATA, SW_CLOCK, SW_LATCH);

#define SSEG_DATA (5)
#define SSEG_CLOCK (6)
#define SSEG_LATCH (7) 
SSeg sseg(SSEG_DATA, SSEG_CLOCK, SSEG_LATCH);

#define BL_DATA   (24)
#define BL_CLOCK  (12)
#define BL_LATCH  (4)
ParallelOutputs button_leds(BL_DATA, BL_CLOCK, BL_LATCH);

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


//const int latchPin = 7;
//const int clockPin = 6;
//const int dataPin = 5;

void setup() {
  // put your setup code here, to run once:
  //  pinMode(PIN_DIR, OUTPUT);
  //  pinMode(PIN_PULSE, OUTPUT);
  //  pinMode(36, INPUT_PULLUP);
  //  pinMode(35, INPUT_PULLUP);

  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);

//button_leds.begin();
//  pinMode(latchPin, OUTPUT);
//  pinMode(dataPin, OUTPUT);  
//  pinMode(clockPin, OUTPUT);

  //  button_start.attach(PIN_SW1, INPUT_PULLUP);
  //  button_stop.attach(PIN_SW2, INPUT_PULLUP);

  Serial.begin(9600);
  shell_init(shell_reader, shell_writer, PSTR("Hi-Proof elevatormatic"));
  
//  shell_register(command_home, PSTR("home"));
//  shell_register(command_info, PSTR("info"));
//  shell_register(command_open, PSTR("open"));
//  shell_register(command_close, PSTR("close"));
//  shell_register(command_move, PSTR("move"));

  // shell_register(command_go, PSTR("g"));
  // shell_register(command_back, PSTR("b"));
  // shell_register(command_halt, PSTR("h"));
  // shell_register(command_estop, PSTR("e"));
  // shell_register(command_motor, PSTR("m"));
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


// FancyButton b1(
//   ParallelBounce(buttons, 0),
//   ParallelOutputPin(button_leds, 3)
// );

// FancyButton b2(
//   ParallelBounce(buttons, 1),
//   ParallelOutputPin(button_leds, 2)
// );

// FancyButton b3(
//   ParallelBounce(buttons, 2),
//   ParallelOutputPin(button_leds, 1)
// );

// FancyButton b4(
//   ParallelBounce(buttons, 3),
//   ParallelOutputPin(button_leds, 0)
// );

// FancyButton b5(
//   ParallelBounce(buttons, 7),
//   ParallelOutputPin(button_leds, 4)
// );

// FancyButton b6(
//   ParallelBounce(buttons, 6),
//   ParallelOutputPin(button_leds, 5)
// );

// FancyButton& button_star = b1;
// FancyButton& button_13f = b2;
// FancyButton& button_14f = b3;
// FancyButton& button_bell = b4;
// FancyButton& button_close = b5;
// FancyButton& button_open = b6;

void on_press() {
  Serial.printf("Pressed\r\n");
}

void on_hold() {
  Serial.printf("Held\r\n");
}

static bool maintenance_mode = false;
static int32_t position_star = 0;
static int32_t position_13f = 0;
static int32_t position_14f = 0;

void maint_start() 
{
  maintenance_mode = true;
  controller.stop();
}

using FloorNames = hiproof::elevator::CallPanel;

void EnterMaintenanceMode() {
  floor_ctrl.emergencyStop();
  current_state = DeviceStates::FloorProgramming;
  call_panel.setButtonCallback(FloorNames::Open, []() -> void {
    call_panel.setSevenSegment('<', '>');
    floor_ctrl.rotate(true);
  });

  call_panel.setButtonCallback(FloorNames::Close, []() -> void {
    call_panel.setSevenSegment('>', '<');
    floor_ctrl.rotate(false);
  });

  call_panel.setButtonCallback(FloorNames::Floor12, nullptr,
                               [](int t) -> void { floor_ctrl.setStop(0); });
  call_panel.setButtonCallback(FloorNames::Floor13, nullptr,
                               [](int t) -> void { floor_ctrl.setStop(1); });
  call_panel.setButtonCallback(FloorNames::Floor14, nullptr,
                               [](int t) -> void { floor_ctrl.setStop(2); });

  call_panel.setButtonCallback(FloorNames::Bell,
                               []() -> void {
                                 call_panel.setSevenSegment('-','-');
                                 floor_ctrl.stopRotation();
                               },
                               [](int t) -> void {
                                 floor_ctrl.emergencyStop();
                                 ExitMaintenanceMode();
                               });
}

void ExitMaintenanceMode() { 
  floor_ctrl.emergencyStop();
  current_state = DeviceStates::Normal;
  call_panel.setButtonCallback(FloorNames::Open, []() -> void {
    call_panel.setSevenSegment('<', '>');
    floor_ctrl.rotate(true);
  });

  call_panel.setButtonCallback(FloorNames::Close, []() -> void {
    call_panel.setSevenSegment('>', '<');
    floor_ctrl.rotate(false);
  });

  call_panel.setButtonCallback(FloorNames::Floor12, []() -> void {
    call_panel.setSevenSegment(12);
    floor_ctrl.moveToStop(0);
  });
  call_panel.setButtonCallback(FloorNames::Floor13, []() -> void {
    call_panel.setSevenSegment(13);
    floor_ctrl.moveToStop(1);
  });
  call_panel.setButtonCallback(FloorNames::Floor14, []() -> void {
    call_panel.setSevenSegment(14);
    floor_ctrl.moveToStop(2);
  });

  call_panel.setButtonCallback(FloorNames::Bell,
                               []() -> void { floor_ctrl.emergencyStop(); },
                               [](int t) -> void { EnterMaintenanceMode(); });
}

void maint_stop() 
{
  maintenance_mode = false;
  rc.stop();
}

void loop() {
  buttons.update();

  if (current_state == DeviceStates::FloorProgramming) {
    // button_bell.on = (millis() / 500) % 2 == 0;
    // button_open.on = true;
    // button_close.on = true;

    sseg.values[0] = SEG_G;
    sseg.values[1] = SEG_G;

    // if (button_bell.on) {
    //   sseg.values[0] |= SEG_DP;
    // } else {
    //   sseg.values[0] &= ~SEG_DP;
    // }

    
  } else {
    // button_bell.on = false;
    // button_open.on = false;
    // button_close.on = false;

    sseg.values[0] &= ~SEG_DP;

  }
  
  button_leds.update();
  sseg.update();
  floor_ctrl.update();
  call_panel.update();

  //Serial.println(buttons.values, BIN);
  //  Serial.printf("SW1: %d   SW2: %d\r\n", digitalRead(PIN_SW1), digitalRead(PIN_SW2));
  ///delay(500);

//  digitalWrite(latchPin, LOW);
//  shiftOut(dataPin, clockPin, MSBFIRST, (millis() / 100) & 0xFF);
//  shiftOut(dataPin, clockPin, MSBFIRST, (millis() / 100) & 0xFF);
//  shiftOut(dataPin, clockPin, MSBFIRST, (millis() / 100) & 0xFF);
//  digitalWrite(latchPin, HIGH);


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

// int command_home(int argc, char** argv)
// {
//   shell_printf("Starting D1 homing cycle\r\n");
//   d1.homing_cycle();
//   shell_printf("D1 closed position: %d\r\n", d1.pos_closed);

//   shell_printf("Starting D2 homing cycle\r\n");
//   d2.homing_cycle();
//   shell_printf("D1 closed position: %d\r\n", d2.pos_closed);

//   return SHELL_RET_SUCCESS;
// }

// int command_info(int argc, char ** argv)
// {
//   shell_printf("D1 SW1: %d   SW2: %d   Pos: %d\r\n", digitalRead(d1.pin_sw1), digitalRead(d1.pin_sw2), d1.s.getPosition());
//   shell_printf("D2 SW1: %d   SW2: %d   Pos: %d\r\n", digitalRead(d2.pin_sw1), digitalRead(d2.pin_sw2), d2.s.getPosition());
//   return SHELL_RET_SUCCESS;
// }

// int command_open(int argc, char ** argv)
// {
//   d1.s.setTargetAbs(0);
//   d2.s.setTargetAbs(0);
//   controller.move(d1.s, d2.s);
//   return SHELL_RET_SUCCESS;
// }

// int command_close(int argc, char ** argv)
// {
//   d1.s.setTargetAbs(d1.pos_closed);
//   d2.s.setTargetAbs(d2.pos_closed);
//   controller.move(d1.s, d2.s);
//   return SHELL_RET_SUCCESS;
// }

// int command_move(int argc, char ** argv) {
//   if (argc >= 2) {
//     int new_pos = atoi(argv[1]);
//     d1.s.setTargetRel(new_pos);
//     controller.move(d1.s);
//   }
// }

// int command_go(int argc, char ** argv)
// {
//    d1.s.setMaxSpeed(motor_speed);
//    d1.s.setAcceleration(motor_accel);
//    rc.rotateAsync(d1.s);
//    return SHELL_RET_SUCCESS;
// }

// int command_back(int argc, char ** argv)
// {
//   d1.s.setMaxSpeed(-motor_speed);
//   d1.s.setAcceleration(motor_accel);
//    rc.rotateAsync(d1.s);
//    return SHELL_RET_SUCCESS;
// }

// int command_halt(int argc, char ** argv)
// {
//    rc.stopAsync();
//    return SHELL_RET_SUCCESS;
// }

// int command_estop(int argc, char ** argv)
// {
//    rc.emergencyStop();
//    return SHELL_RET_SUCCESS;
// }


// int command_motor(int argc, char** argv)
// {
//   if (argc == 1) {
//     shell_printf("Max speed:  %d\r\n", motor_speed);
//     shell_printf("Max accel:  %d\r\n", motor_accel);
//     shell_printf("Position:   %d\r\n", d1.s.getPosition());
//   }
//   if (argc >= 2) {
//     if (0 == strcmp(argv[1], "speed")) {
//       if (argc >= 3) {
//         motor_speed = atoi(argv[2]);
//       }
//       shell_printf("Max speed:  %d\r\n", motor_speed);
//     }
//     if (0 == strcmp(argv[1], "accel")) {
//       if (argc >= 3) {
//         motor_accel = atoi(argv[2]);        
//       }
//       shell_printf("Max accel:  %d\r\n", motor_accel);
//     }
// //    if (0 == strcmp(argv[1], "start")) {
// //      rc.rotateAsync(s1);
// //    }
// //    if (0 == strcmp(argv[1], "stop")) {
// //      rc.stopAsync();
// //    }
// //    if (0 == strcmp(argv[1], "e")) {
// //      rc.emergencyStop();
// //    }
// //
//   }
//   return SHELL_RET_SUCCESS;
// }

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
