/** Implementation details for the call panel hardware
 * 
 */

#include "call_panel.h"

using namespace hiproof::elevator;

CallPanel::CallPanel() : seven_seg{default_sseg} {
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
