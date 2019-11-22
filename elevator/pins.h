#pragma once

#define SW_DATA (26)
#define SW_CLOCK (27)
#define SW_LATCH (25)

#define BL_DATA   (24)
#define BL_CLOCK  (12)
#define BL_LATCH  (4)

#define SSEG_DATA (5)
#define SSEG_CLOCK (6)
#define SSEG_LATCH (7) 

enum {
  PIN_BOARD_ID = 15,
  PIN_TEENSY_LED = 13,

  PIN_UV = 14,
  PIN_R = 16,
  PIN_G = 17,
  PIN_B = 20,
  PIN_W = 21,
};
