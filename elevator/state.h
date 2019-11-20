#pragma once

#include <stdint.h>

enum {
  MODE_NOT_CONNECTED = 0,
  MODE_MEASUREMENT,
  MODE_ONLINE,
  MODE_OFFLINE
};

enum {
  DOOR_NOT_CALIBRATED,
  DOOR_OPEN,
  DOOR_OPENING,
  DOOR_CLOSED,
  DOOR_CLOSING,
  DOOR_LOCKED
};

enum {
  MSG_INNER_STATE,
  MSG_OUTER_STATE,
  MSG_EVENT_BUTTON,
  MSG_EVENT_DOOR,
  MSG_HOME,
  MSG_OPEN,
  MSG_CLOSE,
  MSG_UI_OVERRIDE,
};

enum {
  LOCATION_LOBBY,
  LOCATION_13F,
  LOCATION_14F
};

enum {
  DEST_ARRIVED,
  DEST_MOVING,
  DEST_QUEUED
};

enum {
  BTN1 = 0b000001,
  BTN2 = 0b000010,
  BTN3 = 0b000100,
  BTN4 = 0b001000,
  BTN5 = 0b010000,
  BTN6 = 0b100000,
};

enum {
  BTN_PRESS = 0b10000000,
  BTN_HOLD  = 0b01000000
};

#pragma pack(push, 1)

typedef struct {
  uint8_t door_state;
  uint8_t buttons_state;
} inner_state_t;

typedef struct {
  uint8_t system_mode;
  uint8_t door_state;
  uint8_t destination;
  uint8_t call_button_pressed;
} outer_state_t;

typedef struct {
  uint8_t button;
  uint8_t all_buttons;
} event_button_t;

typedef struct {
  uint8_t door_state;
} event_door_t;

typedef struct {
  uint8_t display[2];
  uint16_t duration;
} ui_override_t;

#pragma pack(pop)
