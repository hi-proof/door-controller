#pragma once

#include <stdint.h>

enum {
  MODE_NOT_CALIBRATED = 0,
  MODE_MAINTENANCE,
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
  FLOOR_NOT_CALIBRATED,
  FLOOR_MOVING,
  FLOOR_MAINTAINING,
  FLOOR_STATIONARY
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
  LOCATION_14F,
  //----------
  LOCATION_COUNT,
};

enum {
  DEST_ARRIVED,
  DEST_MOVING,
  DEST_QUEUED
};

enum {
  BTN_CALL = 0,
  BTN_STAR = 0,
  BTN_13,
  BTN_14,
  BTN_BELL,
  BTN_CLOSE,
  BTN_OPEN,  
};

enum {  
  BTN_PRESS   = 1,
  BTN_RELEASE,
  BTN_CLICK,
  BTN_HOLD,
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
  uint8_t button;       // 4 high bits are event id, 4 low bits are button id
  uint8_t all_buttons;  // bitmask of all buttons
} event_button_t;

typedef struct {
  uint8_t door_state;
} event_door_t;

typedef struct {
  uint8_t display[2];
  uint16_t duration;
} ui_override_t;

#pragma pack(pop)
