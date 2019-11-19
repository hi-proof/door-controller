/** Interface specification for the call panel
 *
 * @todo Question whether we need two classes for the two panels
 *
 */

#pragma once

#ifndef HI_PROOF_CALL_PANEL_H_
#define HI_PROOF_CALL_PANEL_H_

#include "parallelio.h"
#include "elevator_config.h"

namespace hiproof {
namespace elevator {

class CallPanel final {
 public:
  enum ButtonNames {
    Floor12 = 0,
    Floor13,
    Floor14,
    Bell,
    Close,
    Open,
    _MAX_BUTTONS,
  };

  using ButtonPressCallback = void (*)(void);
  using ButtonHoldCallback = void (*)(int);

  CallPanel();
  ~CallPanel() {}

  void setButtonCallback(ButtonNames button,
                         ButtonPressCallback on_press = nullptr,
                         ButtonHoldCallback on_hold = nullptr);
  void setButton(ButtonNames button, FancyButton new_button);
  void setButtonLED(ButtonNames button, bool value);

  void update();

  void setSevenSegment(uint8_t value);
  void setSevenSegment(char lchar, char rchar);

 private:
  FancyButton button_map_[_MAX_BUTTONS];
  ButtonPressCallback press_callbacks_[_MAX_BUTTONS];
  ButtonHoldCallback hold_callbacks_[_MAX_BUTTONS];
  SSeg seven_seg_{default_sseg};
};

}  // namespace elevator
}  // namespace hiproof

#endif  // HI_PROOF_CALL_PANEL_H_