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
bool homing_done = false;
bool homing_in_progress = false;

//--------------------------------------------------------------------------


StepControl doors_controller(25);
StepControl floor_controller(25);

// used for homing only
RotateControl rc(25);

Door d1(38, 37, 39, 32);
Door d2(30, 31, 28, 29);
Floor susan(34, 33, 35, 36);

void tx_msg(uint8_t msg, uint8_t * buffer, uint8_t size) {
  static uint8_t msg_buffer[255];
  if (size > 254) {
    size = 254;
  }
  msg_buffer[0] = msg;
  memcpy(&msg_buffer[1], buffer, size);
  b2b_comms.send(msg_buffer, size + 1);
}

void do_homing()
{  
  homing_in_progress = true;
  
  d1.homing_cycle(rc);
  d1.s.setTargetAbs(0);
  doors_controller.moveAsync(d1.s);
  d2.homing_cycle(rc);
  d2.s.setTargetAbs(0);
  doors_controller.moveAsync(d2.s);

  homing_in_progress = false;
  homing_done = true;
}

void do_open()
{
  if (!homing_done) {
    return;
  }
  
  doors_controller.stop();
  d1.s.setTargetAbs(0);
  d2.s.setTargetAbs(0);
  doors_controller.moveAsync(d1.s, d2.s);
}

void do_close()
{
  if (!homing_done) {
    return;
  }

  doors_controller.stop();
  d1.s.setTargetAbs(d1.pos_closed);
  d2.s.setTargetAbs(d2.pos_closed);
  doors_controller.moveAsync(d1.s, d2.s);
}

void on_rx_button_event(event_button_t * evt)
{
  bool is_hold = evt->button & BTN_HOLD;
  bool is_press = evt->button & BTN_HOLD;

  if (evt->button & BTN4) {
    if (is_hold) {
      if (!homing_done) {
          tx_msg(MSG_HOME, NULL, 0);
          delay(100);
          do_homing();
      }
    }
  }

  if (evt->button & BTN6) {
    // open
    if (homing_done) {
      tx_msg(MSG_OPEN, NULL, 0);
      delay(100);
      do_open();
    }
  }

  if (evt->button & BTN5) {
    // close
    if (homing_done) {
      tx_msg(MSG_CLOSE, NULL, 0);
      delay(100);
      do_close();
    }
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


void on_rx_inner(const uint8_t* buffer, size_t size)
{
  switch (buffer[0]) {
    case MSG_OUTER_STATE: break;
    case MSG_OPEN: 
      if (homing_done) {
        do_open();
      }
      break;
    case MSG_CLOSE: 
      if (homing_done) {
        do_close();
      }
      break;
    case MSG_HOME: 
      do_homing();
      break;
    case MSG_UI_OVERRIDE: break;
  } 
}

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
 
  // LED indicator on teensy
  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);

  // USB serial for debug
  Serial.begin(9600);
  shell_init(shell_reader, shell_writer, PSTR("Hi-Proof elevatormatic"));

  if (is_outer) {
    outer_state.system_mode = MODE_NOT_CONNECTED;
  }
    
  shell_register(command_info, PSTR("i"));
  shell_register(command_home, PSTR("home"));
}

void process_outer() 
{
  if (panel.b1.held && !homing_done) {
    tx_msg(MSG_HOME, NULL, 0);
    delay(100);
    do_homing();
  }
}

void send_button_events(FancyButton& b, uint8_t button_id) {
  event_button_t button_event;
  button_event.all_buttons = panel.buttons_state();

  if (b.pressed) {
    button_event.button = button_id | BTN_PRESS;
    shell_printf("Sending button event %d\r\n", button_id);
    tx_msg(MSG_EVENT_BUTTON, (uint8_t*)&button_event, sizeof(button_event));
  }  
  if (b.held) {
    shell_printf("Sending button event hold\r\n");
    button_event.button = button_id | BTN_HOLD;
    tx_msg(MSG_EVENT_BUTTON, (uint8_t*)&button_event, sizeof(button_event));
  }  
}

void process_inner() 
{
  // send key press events
  send_button_events(panel.b1, BTN1);
  send_button_events(panel.b2, BTN2);
  send_button_events(panel.b3, BTN3);
  send_button_events(panel.b4, BTN4);
  send_button_events(panel.b5, BTN5);
  send_button_events(panel.b6, BTN6);
}

void loop() {
  // update inputs
  buttons.update();
  b2b_comms.update();
  panel.update();
  
  // process
  shell_task();
  
  if (is_outer) {
    process_outer();
  } else {
    process_inner();
  }

  //UI
  if (!homing_done) {
    sseg.set((homing_in_progress) ? SSeg::character('H') | SEG_DP : SSeg::character('H'), 0);
  } else {
    sseg.set(SSeg::character('-'), SSeg::character('-'));
  }

  // update outputs
  button_leds.update();
  sseg.update();

//  // send state if enough time passed
  if (millis() - last_tx > 1000) {
    if (is_outer) {
      tx_msg(MSG_OUTER_STATE, (uint8_t*)&outer_state, sizeof(outer_state));
    } else {
      tx_msg(MSG_INNER_STATE, (uint8_t*)&inner_state, sizeof(inner_state));
    }
    last_tx = millis();
  }
}

//-------------------------------------------------------------------------------------------
// CLI

int command_info(int argc, char ** argv)
{
  bool outer = digitalRead(15) == HIGH;
  shell_printf("Board id: %s\r\n", outer ? "OUTER" : "INNER");
  return SHELL_RET_SUCCESS;
}

int command_home(int argc, char ** argv)
{
  do_homing();
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
