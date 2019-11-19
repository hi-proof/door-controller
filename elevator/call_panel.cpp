/** Implementation details for the call panel hardware
 * 
 */

#include "call_panel.h"

using namespace hiproof::elevator;

CallPanel::CallPanel() {
  button_map_[Floor12] = FancyButton(ParallelBounce(default_buttons, 0),
                                     ParallelOutputPin(default_button_leds, 3));
  button_map_[Floor13] = FancyButton(ParallelBounce(default_buttons, 1),
                                     ParallelOutputPin(default_button_leds, 2));
  button_map_[Floor14] = FancyButton(ParallelBounce(default_buttons, 2),
                                     ParallelOutputPin(default_button_leds, 1));
  button_map_[Bell] = FancyButton(ParallelBounce(default_buttons, 3),
                                     ParallelOutputPin(default_button_leds, 0));
  button_map_[Close] = FancyButton(ParallelBounce(default_buttons, 7),
                                     ParallelOutputPin(default_button_leds, 4));
  button_map_[Open] = FancyButton(ParallelBounce(default_buttons, 6),
                                     ParallelOutputPin(default_button_leds, 5));
}

void CallPanel::setButtonCallback(ButtonNames button,
                       ButtonPressCallback on_press,
                       ButtonHoldCallback on_hold) {
  press_callbacks_[button] = on_press;
  hold_callbacks_[button] = on_hold;
  button_map_[button].update(on_press, on_hold);
}
void CallPanel::setButton(ButtonNames button, FancyButton new_button) {
  button_map_[button] = FancyButton(new_button);
}
void CallPanel::setButtonLED(ButtonNames button, bool value) {
  button_map_[button].on = value;
  button_map_[button].update();
}

void CallPanel::update() {
  default_buttons.update();
  for (int i = 0; i < _MAX_BUTTONS; ++i) {
    button_map_[i].update(press_callbacks_[i], hold_callbacks_[i]);
  }
  // button_map_.update();
  seven_seg_.update();
  default_button_leds.update();
}

void CallPanel::setSevenSegment(uint8_t value) {
  seven_seg_.values[0] = SSeg::digit(value / 10);
  seven_seg_.values[1] = SSeg::digit(value % 10);
  seven_seg_.update();
}

void CallPanel::setSevenSegment(char lchar, char rchar) {
  seven_seg_.values[0] = SSeg::character(lchar);
  seven_seg_.values[1] = SSeg::character(rchar);
  seven_seg_.update();
}
