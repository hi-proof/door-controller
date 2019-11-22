#include <PacketSerial.h>
#include <TeensyStep.h>
#include <Shell.h>
#include <Bounce2.h>
#include <FastLED.h>
#include <SD.h>
#include <Audio.h>
#include <EEPROM.h>

#include "parallelio.h"
#include "state.h"
#include "controllers.h"

//--------------------------------------------------------------------------


// GUItool: begin automatically generated code
AudioPlaySdRaw           playBackground;     //xy=357,267
AudioPlayMemory          playDings;       //xy=367,198
AudioMixer4              mixerTop;         //xy=627,279
AudioMixer4              mixerBottom;         //xy=629,365
AudioOutputI2S           i2s1;           //xy=815,281
AudioOutputUSB           usb1;           //xy=815,353
AudioConnection          patchCord1(playBackground, 0, mixerTop, 1);
AudioConnection          patchCord2(playBackground, 0, mixerBottom, 1);
AudioConnection          patchCord3(playDings, 0, mixerTop, 0);
AudioConnection          patchCord4(playDings, 0, mixerBottom, 0);
AudioConnection          patchCord5(mixerTop, 0, i2s1, 0);
AudioConnection          patchCord6(mixerTop, 0, usb1, 0);
AudioConnection          patchCord7(mixerBottom, 0, i2s1, 1);
AudioConnection          patchCord8(mixerBottom, 0, usb1, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=639,160
// GUItool: end automatically generated code


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

    uint8_t mode;

    uint8_t doors_state;

    uint8_t floor_state;
    uint8_t destination_id;
    int32_t destination_pos;
    int32_t origin_pos;

    int32_t positions[LOCATION_COUNT];

    Elevator(bool with_floor=false)
    : rc(25),
      susan(34, 33, 35, 36, rc), // port A
      d1(38, 37, 39, 32, rc),    // port B
      d2(30, 31, 28, 29, rc),    // port C
      with_floor(with_floor),
      doors_state(DOOR_NOT_CALIBRATED)
    {
      positions[LOCATION_LOBBY] = 0; //load_location(LOCATION_LOBBY);
      positions[LOCATION_13F] = 13000; //load_location(LOCATION_13F);
      positions[LOCATION_14F] = 24000; //load_location(LOCATION_14F);
    }

    int32_t load_location(uint8_t location_id) {
      int32_t ret;
      EEPROM.get(location_id * sizeof(int32_t), ret);
      return ret;
    }

    int32_t save_location(uint8_t location_id, int32_t position) {
      EEPROM.put(location_id, position);
    }

    bool calibrated()
    {
      bool ret = d1.calibrated && d2.calibrated;
      if (with_floor) {
        ret &= susan.calibrated;
      }
      return ret;
    }

    void set_mode(uint8_t new_mode) {
      switch (new_mode) {
        case MODE_MAINTENANCE:
          open();
          susan.stop(true);
          break;

        case MODE_ONLINE:
          susan.stop(true);
          break;

        case MODE_OFFLINE:
          goto_destination(LOCATION_LOBBY);
          open();
          break;
      }
    }

    // hella blocking
    void calibrate()
    {
      shell_printf("Starting elevator calibration\r\n");
      d1.calibrate();
      d1.open();
      d2.calibrate();
      d2.open();
      doors_state = DOOR_OPEN;
      if (with_floor) {
        shell_printf("Calibrating floor\r\n");
        susan.calibrate();
        floor_state = FLOOR_STATIONARY;
        destination_id = LOCATION_LOBBY;
        destination_pos = 0;
        origin_pos = 0;
      }
      mode = MODE_ONLINE;
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

    void open() {
      doors(true);
    }

    void close() {
      doors(false);
    }

    void goto_destination(uint8_t destination_id) {
      // for now - ignore we aren't stationary
      if (floor_state != FLOOR_STATIONARY) {
        shell_printf("Wait for elevator to stop before moving\r\n");
        return;
      }

      // already there?
      if (destination_id == this->destination_id) {
        return;
      }

      this->destination_id = destination_id;
      this->destination_pos = positions[destination_id];
      this->origin_pos = susan.s.getPosition();
      floor_state = FLOOR_MOVING;
      susan.goto_pos(destination_pos);
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
    if (event_id == BTN_HOLD && evt->all_buttons & BTN_STAR && evt->all_buttons & BTN_13 && evt->all_buttons & BTN_14) {
      elevator.mode = MODE_MAINTENANCE;
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
    delay(50);
    elevator.open();
  }

  if (button_id == BTN_CLOSE && event_id == BTN_PRESS) {
    tx_msg(MSG_CLOSE, NULL, 0);
    delay(50);
    elevator.close();
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
      elevator.open();
      break;
      
    case MSG_CLOSE: 
      elevator.close();
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
  
  elevator.with_floor = true; //is_outer;

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

  AudioMemory(100);
  shell_printf("Opening SD...");
  if (!SD.begin(BUILTIN_SDCARD)) {
    shell_printf("Error opening SD\r\n");
  } else {
    shell_printf("Done\r\n");
  }

  sgtl5000_1.enable();
  sgtl5000_1.volume(1.0);

  mixerTop.gain(0, 1.0);
  mixerTop.gain(1, 1.0);
  mixerTop.gain(2, 1.0);
  mixerTop.gain(3, 1.0);

  mixerBottom.gain(0, 1.0);
  mixerBottom.gain(1, 1.0);
  mixerBottom.gain(2, 1.0);
  mixerBottom.gain(3, 1.0);
}

void process_outer() 
{
  // call button held when we're not calibrated yet
  if (panel.button_call.held && !elevator.calibrated()) {
    tx_msg(MSG_HOME, NULL, 0);
    delay(50);
    elevator.calibrate();
  }

  // elevator position handling for ambiance changes
  if (elevator.floor_state == FLOOR_MOVING) {
    int32_t current_position = elevator.susan.s.getPosition();
    // dest is elevator.destination_pos
    // origin is elevator.origin_pos
  }
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
    //command_info(0, NULL);
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
  elevator.goto_destination(LOCATION_13F);
  
  //playBackground.play("LEVEL0.RAW");
//  elevator.calibrate();
//  elevator.susan.rotate(true);
  
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
