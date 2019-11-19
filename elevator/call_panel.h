/** Interface specification for the call panel
 *
 * @todo Question whether we need two classes for the two panels
 *
 */

#pragma once

#ifndef HI_PROOF_CALL_PANEL_H_
#define HI_PROOF_CALL_PANEL_H_

#include "parallelio.h"

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
    MAX_BUTTONS,
  };

  using ButtonPressCallback = void (*)(void);
  using ButtonHoldCallback = void (*)(int);

  CallPanel();
  ~CallPanel() {}

  void setButtonCallback(ButtonNames button,
                         ButtonPressCallback on_press = nullptr,
                         ButtonHoldCallback on_hold = nullptr) {
    press_callbacks_[button] = on_press;
    hold_callbacks[button] = on_hold;
    button_map_[button].update(on_press, on_hold);
  }
  void setButton(ButtonNames button, FancyButton new_button) {
    button_map_[button] = FancyButton(new_button);
  }

  void update() {
    for (int i = 0; i < MAX_BUTTONS; ++i) {
      button_map_[i].update(press_callbacks_[i], hold_callbacks[i]);
    }
  }

 private:
  FancyButton button_map_[MAX_BUTTONS];
  ButtonPressCallback press_callbacks_[MAX_BUTTONS];
  ButtonHoldCallback hold_callbacks[MAX_BUTTONS];
};

}  // namespace elevator
}  // namespace hiproof

#endif  // HI_PROOF_CALL_PANEL_H_