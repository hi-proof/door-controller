#include <PacketSerial.h>
#include <TeensyStep.h>
#include <Shell.h>
#include <Bounce2.h>

#include "parallelio.h"
#include "state.h"
#include "controllers.h"

//--------------------------------------------------------------------------

PacketSerial b2b_comms;
uint32_t last_tx;
uint32_t last_rx;

inner_state_t inner_state;
outer_state_t outer_state;

bool is_outer;

//--------------------------------------------------------------------------
// Globals and functions shared between inner and outer MCUs


class Elevator
{
  public:
    Floor susan;
    Door d1;
    Door d2;
    bool with_floor;
    RotateControl rc;

    uint8_t doors_state;

    Elevator(bool with_floor=false)
    : rc(25),
      susan(34, 33, 35, 36, rc), // port A
      d1(38, 37, 39, 32, rc),    // port B
      d2(30, 31, 28, 29, rc),    // port C
      with_floor(with_floor),
      doors_state(DOOR_NOT_CALIBRATED)
    {}

    bool calibrated()
    {
      bool ret = d1.calibrated && d2.calibrated;
      if (with_floor) {
        ret &= susan.calibrated;
      }
      return ret;     
    }

    // hella blocking
    void calibrate(bool with_floor = false)
    {
      shell_printf("Starting elevator calibration\r\n");
      d1.calibrate();
      d1.open();
      d2.calibrate();
      d2.open();
      doors_state = DOOR_OPEN;
      if (with_floor) {
        susan.calibrate();
      }
      
      shell_printf("Elevator calibrated\r\n");
    }

    // blocking if doors are already moving
    void doors(bool open) {
      if (!calibrated()) {
        shell_printf("ERROR tried using doors before calibration\r\n");
        return;
      }

      if (open && doors_state == DOOR_OPENING || !open && doors_state == DOOR_CLOSING) {
        return;
      }
      
      if (open) {
        doors_state = DOOR_OPENING;
        d1.open();
        d2.open();
      } else {
        doors_state = DOOR_CLOSING;
        d1.close();
        d2.close();
      }
    }

    void update() 
    {
      d1.update();
      d2.update();
      susan.update();
      if (doors_state == DOOR_OPENING) {
        if (!d1.sc.isRunning() & !d2.sc.isRunning()) {
          doors_state = DOOR_OPEN;
        }
      }

      if (doors_state == DOOR_CLOSING) {
        if (!d1.sc.isRunning() & !d2.sc.isRunning()) {
          doors_state = DOOR_CLOSED;
        }
      }

      // TODO: same as above for floor
  
    }
};


// Shlomo TODO: will the other timer work if we only init 2 controllers?
//class Elevator 
//{
//  public:
  
//  // used for the doors
//  StepControl doors_controller;
//  // floor
//  StepControl floor_controller;
//  // used for homing only - both doors and floor
//  RotateControl rc;
  
//  Door d1;
//  Door d2;
//  Floor susan;

//  uint8_t doors_state;
//  uint8_t floor_state;

  // --------------------------------------------------------------

//  Elevator()
//    : d1(38, 37, 39, 32),    // Port B
//      d2(30, 31, 28, 29),    // Port C
//      susan(34, 33, 35, 36)  // Port A
//      doors_controller(25),  // Pulse width. Make larger if motors are finicky
//      floor_controller(25),
//      rc(25)
//  {
//    
//  }

  // --------------------------------------------------------------
  // door functions

//  // blocking - using rc
//  void home_doors()
//  {
//    // if doors are moving, lets stop them
//    if (doors_state == DOOR_OPENING || doors_state == DOOR_CLOSING) {
//      doors_controller.stop();
//      doors_state = DOOR_NOT_CALIBRATED;
//    }
//
//    d1.calibrate();
//    d1.open();
//    d2.calibrate();
//    d2.open();
    
//    d1.homing_cycle(rc);
//    d1.s.setTargetAbs(0);
//    // save some time - open the door async while we start homing the other door
//    doors_controller.moveAsync(d1.s);
//    
//    d2.homing_cycle(rc);
//    d2.s.setTargetAbs(0);
//    doors_controller.move(d2.s);
//
//    doors_state = DOOR_OPEN;
 // }

//  // async - using door controller
//  void open_doors()
//  {
//    if (doors_state == DOOR_NOT_CALIBRATED) {
//      shell_printf("ERROR: tried opening doors before homing\r\n");
//      return;
//    }
//    
//    if (doors_state == DOOR_OPENING || doors_state == DOOR_OPEN) {
//      return;
//    }
//
//    doors_state = DOOR_OPENING;
//
//    // this is a no-op if doors aren't moving
//    // blocking, but not for long
//    doors_controller.stop(); 
//    
//    d1.s.setTargetAbs(0);
//    d2.s.setTargetAbs(0);
//    doors_controller.moveAsync(d1.s, d2.s);
//  }
//
//  // async - using door controller
//  void close_doors()
//  {
//    if (doors_state == DOOR_NOT_CALIBRATED) {
//      shell_printf("ERROR: tried closing doors before homing\r\n");
//      return;
//    }
//
//    if (doors_state == DOOR_CLOSING || doors_state == DOOR_CLOSED) {
//      return;
//    }
//
//    doors_state = DOOR_CLOSING;
//
//    // this is a no-op if doors aren't moving
//    // blocking, but not for long
//    doors_controller.stop(); 
//    
//    d1.s.setTargetAbs(d1.pos_closed);
//    d2.s.setTargetAbs(d2.pos_closed);
//    doors_controller.moveAsync(d1.s, d2.s);
//  }
//  
//  // --------------------------------------------------------------
//  // floor functions
//
//  // blocking - using rc
//  void home_floor()
//  {
//    if (floor_state == FLOOR_MOVING) {
//      floor_controller.stop();
//      floor_state = FLOOR_NOT_CALIBRATED;
//    }
//    
//    susan.homing_cycle(rc);
//    floor_state = FLOOR_STATIONARY;
//  }
//
//  // async - using rc
//  void rotate_floor(bool direction)
//  {
//    if (floor_state == FLOOR_MOVING) {
//      // blocking - but not for long
//      floor_controller.stop();
//      floor_state = FLOOR_STATIONARY;
//    }
//    
//    floor_state = FLOOR_MAINTAINING;
//    // blocking - but not for long
//    rc.stop();
//    susan.s.setMaxSpeed(direction ? HOMING_SPEED : -HOMING_SPEED);
//    susan.s.setAcceleration(HOMING_ACCEL);
//    rc.rotateAsync(susan.s);
//  }
//
//  // rc - blocking
//  void halt_floor()
//  {
//    rc.stop();
//    floor_state = FLOOR_STATIONARY;
//  }
//
//  // async - using floor controller
//  void rotate_floor_to(int32_t position)
//  {
//    if (floor_state == FLOOR_NOT_CALIBRATED) {
//      shell_printf("ERROR: tried rotating floor before homing\r\n");
//      return;
//    }
//
//    floor_state = FLOOR_MOVING;
//    floor_controller.stop();
//    susan.s.setMaxSpeed(FLOOR_SPEED);
//    susan.s.setAcceleration(FLOOR_ACCEL);
//    susan.s.setTargetAbs(pos);
//    sc.moveAsync(susan.s);
//  }
//
//  void update()
//  {
//    // handle async movement for floor
//    if (floor_state == FLOOR_MOVING || floor_state == FLOOR_MAINTAINING) {
//      // check limit switches - this is end switch
//      if (susan.sw1_hit() && susan.sw2_hit()) {
//        floor_state == FLOOR_MOVING ? floor_controller.emergencyStop() : rc.emergencyStop();
//        floor_state == FLOOR_NOT_CALIBRATED;
//      }
//
//      // just one switch - we're home
//      else if (susan.sw1_hit()) {
//        floor_state == FLOOR_MOVING ? floor_controller.stop() : rc.stop();
//        floor_state = FLOOR_STATIONARY;
//        susan.setPosition(0);
//      }
//
//      if (floor_state == FLOOR_MOVING) {
//        // if controller is done, change state to stationary
//        // TODO
//      }
//    }
//
//    // handle async movement for doors
//    if (doors_state == DOOR_OPENING) {
//      // if controller is done {
//        doors_state = DOOR_OPEN;
//
//      // } else - controller is not done {
//        // we hit the home switch before we were supposed to - stop the door and reset home position
//        if (d1.sw1_hit()) {
//            d1.s.setPosition(0);
//        }
//        if (d2.sw1_hot()) {
//          d2.s.setPosition(0);
//        }
//      //}
//    }
//  }
//};

Elevator elevator(false);

void tx_msg(uint8_t msg, uint8_t * buffer, uint8_t size) {
  static uint8_t msg_buffer[255];
  if (size > 254) {
    size = 254;
  }
  msg_buffer[0] = msg;
  memcpy(&msg_buffer[1], buffer, size);
  b2b_comms.send(msg_buffer, size + 1);
}


//--------------------------------------------------------------------------
// OUTER CONTROLLER

void on_rx_button_event(event_button_t * evt)
{
  uint8_t event_id = evt->button & 0xF0 >> 4;
  uint8_t button_id = evt->button & 0x0F;
  
  if (button_id == BTN_BELL) {
    if (event_id == BTN_HOLD) {
//      if (!elevator.homing_done) {
//          tx_msg(MSG_HOME, NULL, 0);
//          delay(100);
//          // blocking
//          elevator.home_doors();
//          elevator.home_floor();
//      }
    }
  }

  if (button_id == BTN_OPEN && event_id == BTN_PRESS) {
    tx_msg(MSG_OPEN, NULL, 0);
    delay(100);
//    elevator.open_doors();
  }

  if (button_id == BTN_CLOSE && event_id == BTN_PRESS) {
    tx_msg(MSG_CLOSE, NULL, 0);
    delay(100);
//    elevator.close_doors();
  }
}

void on_rx_outer(const uint8_t* buffer, size_t size)
{
  // we're totally connected now. yay
  if (outer_state.system_mode == MODE_NOT_CONNECTED) {
    outer_state.system_mode = MODE_ONLINE;
  }
  
  switch (buffer[0]) {
    case MSG_INNER_STATE:
      memcpy(&inner_state, buffer[1], sizeof(inner_state));
      break;
      
    case MSG_EVENT_BUTTON:
      on_rx_button_event((event_button_t *)&buffer[1]);
      shell_printf("Button event: %d\r\n", buffer[1]);
      break;
    
    case MSG_EVENT_DOOR:
      // door state changed. do somtin about it
      
      break;
  } 
}


//--------------------------------------------------------------------------
// INNER CONTROLLER


void on_rx_inner(const uint8_t* buffer, size_t size)
{
  switch (buffer[0]) {
    case MSG_OUTER_STATE: break;
    case MSG_OPEN: 
//      elevator.open_doors();
      break;
      
    case MSG_CLOSE: 
     // elevator.close_doors();
      break;
      
    case MSG_HOME: 
      // blocking
      //elevator.home_doors();
      break;
      
    case MSG_UI_OVERRIDE: break;
  } 
}


//--------------------------------------------------------------------------
// SHARED PROCESSING, Setup, loop, etc


void on_rx(const uint8_t* buffer, size_t size)
{
//  shell_printf("RX %d bytes\r\n", size);
  last_rx = millis();
  if (is_outer) {
    on_rx_outer(buffer, size);
  } else {
    on_rx_inner(buffer, size);
  }
}

void setup() {
  // board id
  pinMode(15, INPUT);
  is_outer = digitalRead(15) == HIGH;

  // b2b comms
  Serial1.begin(9600);
  b2b_comms.setStream(&Serial1);
  b2b_comms.setPacketHandler(on_rx);

  // USB serial for debug
  Serial.begin(9600);
  while (!Serial);
  shell_init(shell_reader, shell_writer, PSTR("Hi-Proof elevatormatic"));
 
  // LED indicator on teensy
  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);

    
  shell_register(command_info, PSTR("i"));
  shell_register(command_home, PSTR("home"));
  shell_register(command_test, PSTR("test"));
  shell_register(command_open, PSTR("open"));
  shell_register(command_close, PSTR("close"));
}

void process_outer() 
{
  // call button held when we're not calibrated yet
//  if (panel.button_call.held && !elevator.homing_done) {
//    tx_msg(MSG_HOME, NULL, 0);
//    delay(100);
//    elevator.home_doors();
//    elevator.home_floor();
//  }
}

void send_button_events(FancyButton& b) {
  event_button_t button_event;
  button_event.all_buttons = panel.buttons_state();

  if (b.pressed) {
    button_event.button = b.button_id | (BTN_PRESS << 4);
    tx_msg(MSG_EVENT_BUTTON, (uint8_t*)&button_event, sizeof(button_event));
  }  
  if (b.released) {
    button_event.button = b.button_id | (BTN_RELEASE << 4);
    tx_msg(MSG_EVENT_BUTTON, (uint8_t*)&button_event, sizeof(button_event));
  }  
  if (b.clicked) {
    button_event.button = b.button_id | (BTN_CLICK<< 4);
    tx_msg(MSG_EVENT_BUTTON, (uint8_t*)&button_event, sizeof(button_event));
  }  
  if (b.held) {
    button_event.button = b.button_id | (BTN_HOLD << 4);
    tx_msg(MSG_EVENT_BUTTON, (uint8_t*)&button_event, sizeof(button_event));
  }  
}

void process_inner() 
{
  // send key press events
  send_button_events(panel.b1);
  send_button_events(panel.b2);
  send_button_events(panel.b3);
  send_button_events(panel.b4);
  send_button_events(panel.b5);
  send_button_events(panel.b6);
}

void loop() {
  // update inputs
  buttons.update();
  b2b_comms.update();
  panel.update();
  
  // process
  shell_task();
  elevator.update();
  
  if (is_outer) {
    process_outer();
  } else {
    process_inner();
  }

  //UI
  if (!elevator.calibrated()) {
    sseg.set(SSeg::character('H'), 0);
  } else {
    sseg.set(SSeg::character('-'), SSeg::character('-'));
  }

  // update outputs
  button_leds.update();
  sseg.update();

//  // send state if enough time passed
  if (millis() - last_tx > 1000) {
    command_info(0, NULL);
    if (is_outer) {
      tx_msg(MSG_OUTER_STATE, (uint8_t*)&outer_state, sizeof(outer_state));
    } else {
      tx_msg(MSG_INNER_STATE, (uint8_t*)&inner_state, sizeof(inner_state));
    }
    last_tx = millis();
  }
}

//-------------------------------------------------------------------------------------------
// CLI commands

int command_info(int argc, char ** argv)
{
  bool outer = digitalRead(15) == HIGH;
  //shell_printf("Board id: %s\r\n", outer ? "OUTER" : "INNER");
  shell_printf("F: %d %s   D1: %d %s   D2: %d %s  D: %d\r\n", 
    elevator.susan.s.getPosition(), elevator.susan.sc.isRunning() ? "RUNNING" : "IDLE",
    elevator.d1.s.getPosition(), elevator.d1.sc.isRunning() ? "RUNNING" : "IDLE",
    elevator.d2.s.getPosition(), elevator.d2.sc.isRunning() ? "RUNNING" : "IDLE",
    elevator.doors_state);
  return SHELL_RET_SUCCESS;
}

int command_open(int argc, char ** argv)
{
  elevator.doors(true);
  return SHELL_RET_SUCCESS;
}

int command_close(int argc, char ** argv)
{
  elevator.doors(false);
  return SHELL_RET_SUCCESS;
}


int command_home(int argc, char ** argv)
{
  elevator.calibrate();
  return SHELL_RET_SUCCESS;
}

int command_test(int argc, char ** argv)
{
  elevator.calibrate();
  elevator.susan.rotate(true);
  
//elevator.susan.rotate(true);
//  delay(1000);
//  elevator.susan.stop();
//  elevator.susan.rotate(false);
//  delay(1000);
//  elevator.susan.stop();
  return SHELL_RET_SUCCESS;
}


//-------------------------------------------------------------------------------------------
// CLI boilerplate

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
