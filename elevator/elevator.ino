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
#include "AudioSampleDing.h"

//--------------------------------------------------------------------------


#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// GUItool: begin automatically generated code
AudioPlaySdRaw           playBackground1; //xy=246,298
AudioPlaySdRaw           playBackground2;     //xy=247,339
AudioPlaySdRaw           playBackground3;     //xy=247,379
AudioPlayMemory          playDings;      //xy=256,229
AudioMixer4              mixerTop;       //xy=516,310
AudioMixer4              mixerBottom;    //xy=518,396
AudioOutputI2S           i2s1;           //xy=704,312
AudioOutputUSB           usb1;           //xy=704,384
AudioConnection          patchCord1(playBackground1, 0, mixerTop, 1);
AudioConnection          patchCord2(playBackground1, 0, mixerBottom, 1);
AudioConnection          patchCord3(playBackground2, 0, mixerTop, 2);
AudioConnection          patchCord4(playBackground2, 0, mixerBottom, 2);
AudioConnection          patchCord5(playBackground3, 0, mixerBottom, 3);
AudioConnection          patchCord6(playBackground3, 0, mixerTop, 3);
AudioConnection          patchCord7(playDings, 0, mixerTop, 0);
AudioConnection          patchCord8(playDings, 0, mixerBottom, 0);
AudioConnection          patchCord9(mixerTop, 0, i2s1, 0);
AudioConnection          patchCord10(mixerTop, 0, usb1, 0);
AudioConnection          patchCord11(mixerBottom, 0, i2s1, 1);
AudioConnection          patchCord12(mixerBottom, 0, usb1, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=528,191
// GUItool: end automatically generated code


//--------------------------------------------------------------------------

PacketSerial b2b_comms;
uint32_t last_tx;
uint32_t last_rx;

inner_state_t inner_state;
outer_state_t outer_state;

bool is_outer;

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

    bool goto_pending;
    bool just_arrived;
    bool just_started;
    bool just_opened_closed;
    bool call_button_pressed;

    Elevator(bool with_floor = false)
      : rc(25),
        susan(34, 33, 35, 36, rc), // port A
        d1(38, 37, 39, 32, rc),    // port B
        d2(30, 31, 28, 29, rc),    // port C
        with_floor(with_floor),
        doors_state(DOOR_NOT_CALIBRATED),
        mode(MODE_NOT_CALIBRATED),
        floor_state(FLOOR_NOT_CALIBRATED),
        call_button_pressed(false)
    {
      positions[LOCATION_LOBBY] = load_location(LOCATION_LOBBY);
      positions[LOCATION_13F] = load_location(LOCATION_13F);
      positions[LOCATION_14F] = load_location(LOCATION_14F);
    }

    int32_t load_location(uint8_t location_id) {
      int32_t ret;
      EEPROM.get(location_id * sizeof(int32_t), ret);
      return ret;
    }

    int32_t save_location(uint8_t location_id, int32_t position) {
      positions[location_id] = position;
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
      shell_printf("Setting elevator mode to: %d\r\n", new_mode);
      call_button_pressed = false;
      switch (new_mode) {
        case MODE_MAINTENANCE:
          susan.stop(true);
          open();
          break;

        case MODE_ONLINE:
          susan.stop(true);
          break;

        case MODE_OFFLINE:
          susan.stop(true);
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

      this->goto_pending = true;
      close();
      tx_msg(MSG_CLOSE, NULL, 0);
    }

    void update()
    {
      d1.update();
      d2.update();
      susan.update();
      just_opened_closed = false;
      if (doors_state == DOOR_OPENING) {
        if (!d1.sc.isRunning() & !d2.sc.isRunning()) {
          doors_state = DOOR_OPEN;
          just_opened_closed = true;
        }
      }

      if (doors_state == DOOR_CLOSING) {
        if (!d1.sc.isRunning() & !d2.sc.isRunning()) {
          doors_state = DOOR_CLOSED;
          just_opened_closed = true;
        }
      }


      just_arrived = false;
      just_started = false;

      if (floor_state == FLOOR_STATIONARY && mode == MODE_ONLINE && goto_pending) {
        if ((doors_state == DOOR_CLOSED) && (inner_state.door_state == DOOR_CLOSED)) {
          floor_state = FLOOR_MOVING;
          susan.goto_pos(destination_pos);
          goto_pending = false;
          just_started = true;
        }
      } else if (floor_state == FLOOR_MOVING) {
        if (!susan.sc.isRunning()) {
          floor_state = FLOOR_STATIONARY;
          just_arrived = true;
        }
      }
    }
};

Elevator elevator(false);

//--------------------------------------------------------------------------
// OUTER CONTROLLER

void on_rx_button_event(event_button_t * evt)
{
  uint8_t event_id = evt->button & 0xF0 >> 4;
  uint8_t button_id = evt->button & 0x0F;

  shell_printf("Button event %d %d\r\n", button_id, event_id);

  switch (elevator.mode) {
    case MODE_NOT_CALIBRATED:

      break;

    case MODE_MAINTENANCE:
      if (button_id == BTN_BELL && event_id == BTN_HOLD) {
        elevator.set_mode(MODE_ONLINE);
      }
      if (button_id == BTN_BELL && event_id == BTN_CLICK) {
        elevator.susan.stop(true);
      }
      if (button_id == BTN_OPEN && event_id == BTN_CLICK) {
        elevator.susan.rotate(true);
      }
      if (button_id == BTN_CLOSE && event_id == BTN_CLICK) {
        elevator.susan.rotate(false);
      }
      if (button_id == BTN_STAR && event_id == BTN_HOLD) {
        elevator.save_location(LOCATION_LOBBY, elevator.susan.s.getPosition());
      }
      if (button_id == BTN_13 && event_id == BTN_HOLD) {
        elevator.save_location(LOCATION_13F, elevator.susan.s.getPosition());
      }
      if (button_id == BTN_14 && event_id == BTN_HOLD) {
        elevator.save_location(LOCATION_14F, elevator.susan.s.getPosition());
      }
      break;

    case MODE_ONLINE:
      if (button_id == BTN_BELL && event_id == BTN_HOLD) {
        if (evt->all_buttons & BTN_STAR && evt->all_buttons & BTN_13 && evt->all_buttons & BTN_14) {
          elevator.set_mode(MODE_MAINTENANCE);
        }
        else if (evt->all_buttons & BTN_CLOSE && evt->all_buttons & BTN_OPEN) {
          elevator.calibrate();
        }
        else if (evt->all_buttons & BTN_STAR && evt->all_buttons & BTN_OPEN) {
          elevator.set_mode(MODE_OFFLINE);
        }
      }
      if (button_id == BTN_STAR && event_id == BTN_CLICK) {
        elevator.goto_destination(LOCATION_LOBBY);
      }
      if (button_id == BTN_13 && event_id == BTN_CLICK) {
        elevator.goto_destination(LOCATION_13F);
      }
      if (button_id == BTN_14 && event_id == BTN_CLICK) {
        elevator.goto_destination(LOCATION_14F);
      }
      break;

    case MODE_OFFLINE:
      if (button_id == BTN_BELL && event_id == BTN_HOLD) {
        if (evt->all_buttons & BTN_STAR && evt->all_buttons & BTN_OPEN) {
          elevator.set_mode(MODE_ONLINE);
        }
      }
      break;

  }
  //
  //  if (button_id == BTN_BELL && event_id == BTN_HOLD &&

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
  switch (buffer[0]) {
    case MSG_INNER_STATE:
      memcpy(&inner_state, &buffer[1], sizeof(inner_state));
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

float lerp(float a, float b, float x)
{
  return a + x * (b - a);
}

constexpr int NUM_BACKGROUND_TRACKS = 3;
float background_volumes[NUM_BACKGROUND_TRACKS] = {0};

struct Transitions {
  float source_volumes[NUM_BACKGROUND_TRACKS];
  float dest_volumes[NUM_BACKGROUND_TRACKS];
  int32_t start_time;
  int32_t duration;

  Transitions() :
    source_volumes( {
    0
  }), dest_volumes({1, 0, 0}), start_time(0), duration(1) {

  }

  void start_transition(uint8_t target_floor) {
    // copy current audio state as start
    memcpy(source_volumes, background_volumes, sizeof(source_volumes));
    memset(dest_volumes, 0, sizeof(dest_volumes));
    dest_volumes[target_floor] = 1.;

    start_time = millis();
    // TODO: vary according to target/source
    duration = 3000;
  }

  void update() {
    float relative_time = (millis() - start_time) / (float)duration;
    if ((relative_time >= 0) && (relative_time <= 1.)) {
      for (int i = 0; i < NUM_BACKGROUND_TRACKS; i++) {
        background_volumes[i] = lerp(source_volumes[i], dest_volumes[i], relative_time);
      }
      //      shell_printf("rel time: %d, bv: %d %d %d\r\n",
      //                    (int)(relative_time * 100),
      //                    (int)(background_volumes[0]*100),
      //                    (int)(background_volumes[1]*100),
      //                    (int)(background_volumes[2]*100));
    } else {
      memcpy(background_volumes, dest_volumes, sizeof(background_volumes));
    }

    for (int i = 0; i < NUM_BACKGROUND_TRACKS; i++) {
      // TODO: up/down motion via panning between top and bottom
      mixerTop.gain(i + 1, background_volumes[i]);
      mixerBottom.gain(i + 1, background_volumes[i]);
    }

    if (background_volumes[0] > 0.02 && !playBackground1.isPlaying()) {
      playBackground1.play("LEVEL0.RAW");
    } else if (background_volumes[0] < 0.02 && playBackground1.isPlaying()) {
      playBackground1.stop();
    }

    if (background_volumes[1] > 0.02 && !playBackground2.isPlaying()) {
      playBackground2.play("LEVEL1.RAW");
    } else if (background_volumes[1] < 0.02 && playBackground2.isPlaying()) {
      playBackground2.stop();
    }

    if (background_volumes[2] > 0.02 && !playBackground3.isPlaying()) {
      playBackground3.play("LEVEL2.RAW");
    } else if (background_volumes[2] < 0.02 && playBackground3.isPlaying()) {
      playBackground3.stop();
    }
  }
};

Transitions transitions;

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

    case MSG_TRIGGER_TRANSITION:
      // start transition to target floor
      transitions.start_transition(buffer[1]);
      break;

    case MSG_DING:
      playDings.play(AudioSampleDing);
      break;
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

  elevator.with_floor = is_outer;

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

// relative to motion length, so 1.0 is at motion's end
float transition_positions[LOCATION_COUNT] = {
  0.25,
  0.5,
  0.7
};

// TODO: reset on MSG_HOME
uint8_t transition_target = LOCATION_LOBBY;

void process_outer()
{
  outer_state.door_state = elevator.doors_state;
  outer_state.floor_state = elevator.floor_state;
  outer_state.system_mode = elevator.mode;
  outer_state.destination = elevator.destination_id;
  outer_state.call_button_pressed = elevator.call_button_pressed;

  // call button held when we're not calibrated yet
  if (panel.button_call.held && !elevator.calibrated()) {
    tx_msg(MSG_HOME, NULL, 0);
    delay(50);
    elevator.calibrate();
  }

  if (panel.button_call.clicked && elevator.mode == MODE_ONLINE) {
    elevator.call_button_pressed = true;
  }

  if (elevator.just_arrived) {
    switch (elevator.destination_id) {
      case LOCATION_LOBBY:
        elevator.call_button_pressed = false;
        tx_msg(MSG_OPEN, NULL, 0);
        delay(50);
        elevator.open();
        break;

      case LOCATION_13F:
        tx_msg(MSG_OPEN, NULL, 0);
        break;

      case LOCATION_14F:
        tx_msg(MSG_OPEN, NULL, 0);
        break;
    }
    tx_msg(MSG_DING, NULL, 0);
    shell_printf("Just arrived at location ID: %d, should play ding now\r\n", elevator.destination_id);
  }

  // elevator position handling for ambiance changes
  if (elevator.floor_state == FLOOR_MOVING) {
    int32_t current_position = elevator.susan.s.getPosition();
    // dest is elevator.destination_pos
    // origin is elevator.origin_pos
    uint8_t destination_floor = elevator.destination_id;
    // if not already triggered
    if (destination_floor != transition_target) {
      // calculate our position in the motion, check if we passed the threshold
      float relative_position = (current_position - elevator.origin_pos) / (float)(elevator.destination_pos - elevator.origin_pos);
      // check if we crossed the trigger point
      if (relative_position > transition_positions[destination_floor]) {
        shell_printf("Triggered transition to %d, position %d, origin %d, relative: %d\r\n",
                     elevator.destination_id,
                     current_position,
                     elevator.origin_pos,
                     (int)(relative_position * 100));
        // mark triggered
        transition_target = destination_floor;
        // inform the inside part of the trigger
        tx_msg(MSG_TRIGGER_TRANSITION, &destination_floor, sizeof(destination_floor));
      }
    }
  } else {
    // not moving
    transition_target = elevator.destination_id;
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
    button_event.button = b.button_id | (BTN_CLICK << 4);
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

  inner_state.door_state = elevator.doors_state;
  // audio
  transitions.update();
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
  if ((millis() - last_tx > 1000) || (!is_outer && elevator.just_opened_closed)) {
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
  shell_printf("F: %d %d %s   D1: %d %s   D2: %d %s  D: %d\r\n",
               elevator.floor_state,
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
  //playBackground1.play("LEVEL0.RAW");
  //elevator.goto_destination(LOCATION_13F);
  // test ding
  uint8_t msg = MSG_DING;
  on_rx_inner(&msg, sizeof(msg));
  // test audio transitions
  //  static int index = 0;
  //  transitions.start_transition(index % 3);
  //  index++;
  // test transition detection
  //  switch(index){
  //    case 0:
  //      elevator.goto_destination(LOCATION_13F);
  //      break;
  //    case 1:
  //      elevator.goto_destination(LOCATION_14F);
  //      break;
  //    case 2:
  //      elevator.goto_destination(LOCATION_13F);
  //      break;
  //    case 3:
  //      elevator.goto_destination(LOCATION_LOBBY);
  //      break;
  //    case 4:
  //      elevator.goto_destination(LOCATION_14F);
  //      break;
  //    case 5:
  //      elevator.goto_destination(LOCATION_LOBBY);
  //      break;
  //  }
  //  index = (index + 1) % 6;


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
