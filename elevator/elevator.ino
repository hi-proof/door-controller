#include <PacketSerial.h>
#include <TeensyStep.h>
#include <Shell.h>
#include <Bounce2.h>
#include <FastLED.h>
#include <SD.h>
#include <Audio.h>
#include <EEPROM.h>
#include <FastLED.h>

#include "pins.h"
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
//AudioOutputUSB           usb1;           //xy=704,384
AudioConnection          patchCord1(playBackground1, 0, mixerTop, 1);
AudioConnection          patchCord2(playBackground1, 0, mixerBottom, 1);
AudioConnection          patchCord3(playBackground2, 0, mixerTop, 2);
AudioConnection          patchCord4(playBackground2, 0, mixerBottom, 2);
AudioConnection          patchCord5(playBackground3, 0, mixerBottom, 3);
AudioConnection          patchCord6(playBackground3, 0, mixerTop, 3);
AudioConnection          patchCord7(playDings, 0, mixerTop, 0);
AudioConnection          patchCord8(playDings, 0, mixerBottom, 0);
AudioConnection          patchCord9(mixerTop, 0, i2s1, 0);
//AudioConnection          patchCord10(mixerTop, 0, usb1, 0);
AudioConnection          patchCord11(mixerBottom, 0, i2s1, 1);
//AudioConnection          patchCord12(mixerBottom, 0, usb1, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=528,191
// GUItool: end automatically generated code

//--------------------------------------------------------------------------


PacketSerial b2b_comms;
uint32_t last_tx;
uint32_t last_rx;

inner_state_t inner_state;
outer_state_t outer_state;

bool is_outer;
uint8_t av_mode = AV_LOBBY;

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

    uint8_t pending_dest_id;
    uint8_t destination_id;
    int32_t destination_pos;
    int32_t origin_pos;

    int32_t positions[LOCATION_COUNT];

    bool goto_pending;
    bool just_arrived;
    bool just_started;
    bool call_button_pressed;

    bool force_state_update;

    Elevator(bool with_floor = false)
      : rc(25),
        susan(34, 33, 36, 35, rc), // port A
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
      shell_printf("Saving location %d at %d\r\n", location_id, position);
      positions[location_id] = position;
      EEPROM.put(location_id * sizeof(int32_t), position);
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
      mode = new_mode;
      call_button_pressed = false;
      force_state_update = true;
      switch (new_mode) {
        case MODE_MAINTENANCE:
          susan.stop(true);
          destination_id = 100;
          open();
          tx_msg(MSG_OPEN, NULL, 0);
          break;

        case MODE_ONLINE:
          susan.stop(true);
          break;

        case MODE_OFFLINE:
          susan.stop(true);
          goto_destination(LOCATION_LOBBY);
          open();
          tx_msg(MSG_OPEN, NULL, 0);
          break;
      }
    }

    // hella blocking
    void calibrate()
    {
      shell_printf("Starting elevator calibration\r\n");
      
      d1.calibrate();
      d1.goto_pos(0);
      
      d2.calibrate();
      d2.goto_pos(0);
      
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

      this->pending_dest_id = destination_id;
      this->destination_pos = positions[destination_id];
      this->origin_pos = susan.s.getPosition();

      shell_printf("Going to destination id %d position %d (current position %d)\r\n", destination_id, destination_pos, origin_pos);

      this->goto_pending = true;
      tx_msg(MSG_CLOSE, NULL, 0);
      close();
    }

    void update()
    {
      d1.update();
      d2.update();
      susan.update();
      force_state_update = false;
      if (doors_state == DOOR_OPENING) {
        if (!d1.sc.isRunning() & !d2.sc.isRunning()) {
          doors_state = DOOR_OPEN;
          force_state_update = true;
        }
      }

      if (doors_state == DOOR_CLOSING) {
        if (!d1.sc.isRunning() & !d2.sc.isRunning()) {
          doors_state = DOOR_CLOSED;
          force_state_update = true;
        }
      }

      just_arrived = false;
      just_started = false;

      if (with_floor) {
        if (floor_state == FLOOR_STATIONARY && mode == MODE_ONLINE && goto_pending) {
          if ((doors_state == DOOR_CLOSED) && (inner_state.door_state == DOOR_CLOSED)) {
            destination_id = pending_dest_id;
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
    }
};

Elevator elevator(false);

//--------------------------------------------------------------------------
// OUTER CONTROLLER

void on_rx_button_event(event_button_t * evt)
{
  uint8_t event_id = (evt->button & 0xF0) >> 4;
  uint8_t button_id = (evt->button & 0x0F);
  
  switch (elevator.mode) {
    case MODE_NOT_CALIBRATED:
      if (button_id == BTN_BELL && event_id == BTN_HOLD) {
        tx_msg(MSG_HOME, NULL, 0);
        delay(50);
        elevator.calibrate();
      }
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
        tx_msg(MSG_DING, NULL, 0);
      }
      if (button_id == BTN_13 && event_id == BTN_HOLD) {
        elevator.save_location(LOCATION_13F, elevator.susan.s.getPosition());
        tx_msg(MSG_DING, NULL, 0);
      }
      if (button_id == BTN_14 && event_id == BTN_HOLD) {
        elevator.save_location(LOCATION_14F, elevator.susan.s.getPosition());
        tx_msg(MSG_DING, NULL, 0);
      }
      break;

    case MODE_ONLINE:
      if (button_id == BTN_BELL && event_id == BTN_PRESS) {
        tx_msg(MSG_DING, NULL, 0);
      }
      if (button_id == BTN_BELL && event_id == BTN_HOLD) {
        if ((evt->all_buttons & (1 << BTN_STAR)) && (evt->all_buttons & (1 << BTN_13)) && (evt->all_buttons & (1 << BTN_14))) {
          elevator.set_mode(MODE_MAINTENANCE);
        }
        else if ((evt->all_buttons & (1 << BTN_CLOSE)) && (evt->all_buttons & (1 << BTN_OPEN))) {
          tx_msg(MSG_HOME, NULL, 0);
          delay(100);
          elevator.calibrate();
        }
        else if (evt->all_buttons & (1 << BTN_OPEN)) {
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
      if (button_id == BTN_OPEN && event_id == BTN_PRESS) {
        if (elevator.floor_state == FLOOR_STATIONARY) {
          tx_msg(MSG_OPEN, NULL, 0);
          if (elevator.destination_id == LOCATION_LOBBY) {
            elevator.open();
          }
        }
      }
      if (button_id == BTN_CLOSE && event_id == BTN_PRESS) {
        if (elevator.floor_state == FLOOR_STATIONARY) {
          tx_msg(MSG_CLOSE, NULL, 0);
          if (elevator.destination_id == LOCATION_LOBBY) {
            elevator.close();
          }
        }
      }

      break;

    case MODE_OFFLINE:
      if (button_id == BTN_BELL && event_id == BTN_HOLD) {
        if ((evt->all_buttons & (1 << BTN_STAR)) && (evt->all_buttons & (1 << BTN_OPEN))) {
          elevator.set_mode(MODE_ONLINE);
        }
      }

      if (button_id == BTN_BELL && event_id == BTN_PRESS) {
        tx_msg(MSG_DING, NULL, 0);
      }

      if (button_id == BTN_CLOSE && event_id == BTN_PRESS) {
        av_mode = (av_mode + 1) % AV_MODES_COUNT;
        tx_msg(MSG_AVMODE, (uint8_t*)&av_mode, 1);
      }
      if (button_id == BTN_OPEN && event_id == BTN_PRESS) {
        av_mode = (av_mode - 1) % AV_MODES_COUNT;
        tx_msg(MSG_AVMODE, (uint8_t*)&av_mode, 1);
      }

      break;

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
      playBackground1.play("LEVEL12.RAW");
    } else if (background_volumes[0] < 0.02 && playBackground1.isPlaying()) {
      playBackground1.stop();
    }

    if (background_volumes[1] > 0.02 && !playBackground2.isPlaying()) {
      playBackground2.play("LEVEL13.RAW");
    } else if (background_volumes[1] < 0.02 && playBackground2.isPlaying()) {
      playBackground2.stop();
    }

    if (background_volumes[2] > 0.02 && !playBackground3.isPlaying()) {
      playBackground3.play("LEVEL14.RAW");
    } else if (background_volumes[2] < 0.02 && playBackground3.isPlaying()) {
      playBackground3.stop();
    }
  }
};

Transitions transitions;

void on_rx_inner(const uint8_t* buffer, size_t size)
{
  switch (buffer[0]) {
    case MSG_OUTER_STATE: {
        outer_state_t * new_state = (outer_state_t *)&buffer[1];
        elevator.mode = new_state->system_mode;
        elevator.call_button_pressed = new_state->flags & (1 << FLAG_CALL_BUTTON_PENDING);
        elevator.goto_pending = new_state->flags & (1 << FLAG_GOTO_PENDING);
        elevator.destination_id = new_state->destination;
        elevator.floor_state = new_state->floor_state;
      }
      break;
    case MSG_OPEN:
      elevator.open();
      break;

    case MSG_CLOSE:
      elevator.close();
      break;

    case MSG_HOME:
      // blocking
      elevator.calibrate();
      break;

    case MSG_UI_OVERRIDE: break;

    case MSG_TRIGGER_TRANSITION:
      // start transition to target floor
      transitions.start_transition(buffer[1]);
      switch (buffer[1]) {
        case LOCATION_LOBBY:
          av_mode = AV_LOBBY;
          break;
        case LOCATION_13F:
          av_mode = AV_LAB;
          break;
        case LOCATION_14F:
          av_mode = AV_SPACE;
          break;
      }
      break;

    case MSG_DING:
      playDings.play(AudioSampleDing);
      break;

    case MSG_AVMODE:
      av_mode = buffer[1];
      break;
  }
}


//--------------------------------------------------------------------------
// SHARED PROCESSING, Setup, loop, etc


void on_rx(const uint8_t* buffer, size_t size)
{
  last_rx = millis();
  if (is_outer) {
    on_rx_outer(buffer, size);
  } else {
    on_rx_inner(buffer, size);
  }
}

void setup() {
  // board id
  pinMode(PIN_BOARD_ID, INPUT);
  is_outer = digitalRead(PIN_BOARD_ID) == HIGH;

  elevator.with_floor = is_outer;

  // b2b comms
  Serial1.begin(9600);
  b2b_comms.setStream(&Serial1);
  b2b_comms.setPacketHandler(on_rx);

  // USB serial for debug
  Serial.begin(9600);
  //while (!Serial);
  shell_init(shell_reader, shell_writer, PSTR("Hi-Proof elevatormatic"));

  // LED indicator on teensy
  pinMode(PIN_TEENSY_LED, OUTPUT);
  digitalWrite(PIN_TEENSY_LED, HIGH);

  pinMode(PIN_UV, OUTPUT);
  digitalWrite(PIN_UV, LOW);

  shell_register(command_info, PSTR("i"));
  shell_register(command_home, PSTR("home"));
  shell_register(command_test, PSTR("test"));
  shell_register(command_open, PSTR("open"));
  shell_register(command_close, PSTR("close"));

  if (!is_outer) {
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
  outer_state.flags = (
    (elevator.call_button_pressed << FLAG_CALL_BUTTON_PENDING) |
    (elevator.goto_pending << FLAG_GOTO_PENDING));

  // call button held when we're not calibrated yet
  if (panel.b1.held && !elevator.calibrated()) {
    tx_msg(MSG_HOME, NULL, 0);
    delay(50);
    elevator.calibrate();
  }

  if (panel.b1.clicked && elevator.mode == MODE_ONLINE) {
    shell_printf("Call button pressed, processing\r\n");
    elevator.call_button_pressed = true;
    if (elevator.destination_id == LOCATION_LOBBY) {
      if (elevator.floor_state == FLOOR_STATIONARY) {
        elevator.open();
        tx_msg(MSG_OPEN, NULL, 0);
      }
      // Commenting this out on day two
//    } else {
//      elevator.goto_destination(LOCATION_LOBBY);
    }
  }

  if (elevator.destination_id == LOCATION_LOBBY && elevator.floor_state == FLOOR_STATIONARY) {
    elevator.call_button_pressed = false;
  }

  if (elevator.just_arrived) {
    switch (elevator.destination_id) {
      case LOCATION_LOBBY:
        elevator.call_button_pressed = false;
        elevator.open();
        // fallthrough

      case LOCATION_13F:
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

void RGBW(uint8_t r, uint8_t g, uint8_t b, uint8_t w) 
{
    analogWrite(PIN_R, r);
    analogWrite(PIN_G, g);
    analogWrite(PIN_B, b);
    analogWrite(PIN_W, w);
}

void UV(bool on) 
{
  digitalWrite(PIN_UV, on ? HIGH : LOW);
}

void do_av(uint8_t mode)
{
  bool pending_transition = false;
  static uint32_t pending_transition_time = 0;

  switch (mode) {
    case AV_LOBBY:
      RGBW(0, 0, 0, 255);
      UV(false);
      break;
    
    case AV_GLITCHY: {
      static uint8_t glitch = 0;
      if (random(100) < 5) {
        glitch = random(150, 250);
      }
      RGBW(glitch, 0, 0, 255 - glitch);
      if (glitch > 0) {
        glitch--;
      }
      UV(false);
      }
    break;
    
    case AV_LAB:
      RGBW(beatsin8(40), 0, 0, 0);
      UV(true);
      break;
    
    case AV_SPACE:
      RGBW(0, beatsin8(20), beatsin8(40), 0);
      UV(true);
      break;

    case AV_RAINBOW:
      RGBW(
        beatsin8(20),
        beatsin8(30),
        beatsin8(25),
        beatsin8(17)
      );
      UV(true);
      break;

     case AV_FLASHING_BLUE:
        RGBW(0, 0, beatsin8(20), 100);
        UV(true);
        break;

     case AV_FIRE:
        RGBW(inoise8(millis() / 3), 0, 0, inoise8(millis()));
        UV(false);
        break;
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

//  analogWrite(PIN_R, beatsin8(20));
//  analogWrite(PIN_G, beatsin8(30));
//  analogWrite(PIN_B, beatsin8(25));
//  analogWrite(PIN_W, beatsin8(17));    

  switch (elevator.mode) {
    // offline mode - just rainbowy stuff
    case MODE_OFFLINE:
      //do_av(AV_RAINBOW);
      do_av(av_mode);
      break;

    // flashing blue
    case MODE_NOT_CALIBRATED:
    case MODE_MAINTENANCE:
      do_av(AV_FLASHING_BLUE);
      break;

    case MODE_ONLINE:
      do_av(av_mode);
      break;  
  }

  // turn off all floor buttons if not moving

  inner_state.door_state = elevator.doors_state;
  // audio
  transitions.update();
}

void do_ui()
{
  switch (elevator.mode) {
    case MODE_NOT_CALIBRATED:
      sseg.set(SSeg::character('H'), 0);
      break;
    
    case MODE_ONLINE:
      panel.b1.on = false;
      panel.b2.on = false;
      panel.b3.on = false;
      panel.b4.on = false;
      panel.b5.on = false;
      panel.b6.on = false;

      if (elevator.floor_state == FLOOR_STATIONARY) {
        switch (elevator.destination_id) {
          case LOCATION_LOBBY: sseg.set(SSeg::digit(1), SSeg::digit(2)); break;
          case LOCATION_13F: sseg.set(SSeg::digit(1), SSeg::digit(3)); break; 
          case LOCATION_14F: sseg.set(SSeg::digit(1), SSeg::digit(4)); break;
          default: sseg.set(SSeg::character('-'), SSeg::character('-'));
        }

//        if (elevator.goto_pending) {
//          switch (elevator.pending_dest_id) {
//            case LOCATION_LOBBY: panel.b1.on = (millis() / 700) % 2 == 0; break;
//            case LOCATION_13F: panel.b2.on = (millis() / 700) % 2 == 0; break;
//            case LOCATION_14F: panel.b3.on = (millis() / 700) % 2 == 0; break;
//          }
//        }
      }

      if (elevator.floor_state == FLOOR_MOVING) {
        switch (elevator.destination_id) {
          case LOCATION_LOBBY: panel.b1.on = (millis() / 700) % 2 == 0; break;
          case LOCATION_13F: panel.b2.on = (millis() / 700) % 2 == 0; break;
          case LOCATION_14F: panel.b3.on = (millis() / 700) % 2 == 0; break;
        }
      }
      
      // special case - blink the outside call button and let the inside of the elevator know
      // lobby is waiting
      if (elevator.call_button_pressed) {
        panel.b1.on = (millis() / 700) % 2 == 0;
      }       
      break;

    case MODE_MAINTENANCE:
      sseg.set(SSeg::character('-'), SSeg::character('-'));
      panel.b1.on = false;
      panel.b2.on = false;
      panel.b3.on = false;
      panel.b5.on = true;
      panel.b6.on = true;
      panel.b4.on = (millis() / 700) % 2 == 0;
      break;
    
    case MODE_OFFLINE:
      sseg.set((millis() / 100) & 0xFF, (millis() / 200) & 0xFF);
      panel.b1.on = (millis() / 700) % 2 == 0;
      panel.b2.on = (millis() / 1300) % 2 == 0;
      panel.b3.on = (millis() / 500) % 2 == 0;
      panel.b5.on = (millis() / 1700) % 2 == 0;
      panel.b6.on = (millis() / 950) % 2 == 0;
      panel.b4.on = (millis() / 1100) % 2 == 0;
      break;
  }
}

void loop() {
  // update inputs
  buttons.update();
  b2b_comms.update();
  panel.update_inputs();

  // process
  shell_task();
  elevator.update();

  if (is_outer) {
    process_outer();
  } else {
    process_inner();
  }

  //UI
  do_ui();
  
  // update outputs
  panel.update_outputs();
  button_leds.update();
  sseg.update();

  //  // send state if enough time passed
  if ((millis() - last_tx > 2000) || elevator.force_state_update) {
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
  shell_printf("Board id: %s\r\n", outer ? "OUTER" : "INNER");
  shell_printf("F: %d %d %s   D1: %d %s   D2: %d %s  D: %d\r\n",
               elevator.floor_state,
               elevator.susan.s.getPosition(), elevator.susan.sc.isRunning() ? "RUNNING" : "IDLE",
               elevator.d1.s.getPosition(), elevator.d1.sc.isRunning() ? "RUNNING" : "IDLE",
               elevator.d2.s.getPosition(), elevator.d2.sc.isRunning() ? "RUNNING" : "IDLE",
               elevator.doors_state);
  shell_printf("F SW1: %d  S2: %d\r\n", 
    elevator.susan.sw1_hit(),  elevator.susan.sw2_hit());
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
  tx_msg(MSG_HOME, NULL, 0);
  delay(50);
  elevator.calibrate();
  return SHELL_RET_SUCCESS;
}

int command_test(int argc, char ** argv)
{
//  elevator.susan.rotate(true);
//  delay(1000);
  //playBackground1.play("LEVEL0.RAW");
  //elevator.goto_destination(LOCATION_13F);
  // test ding
//  uint8_t msg = MSG_DING;
//  on_rx_inner(&msg, sizeof(msg));
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
