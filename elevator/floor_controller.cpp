/** Floor controller impl details
 *
 */

#include "floor_controller.h"

using namespace hiproof::elevator;

// For future reference, there are the pulse widths of the controllers
StepControl FloorController::step_controller{25};
RotateControl FloorController::rotate_controller{25};

FloorController::FloorController(const int step_pin, const int dir_pin,
                                 const int home_pin, const int overrun_pin)
    : named_stops_{}, stepper_{step_pin, dir_pin}, home_pin_{home_pin}, overrun_pin_{overrun_pin}
{

}

void FloorController::enterSafeMode() { safe_ = true; };

void FloorController::emergencyStop() {
  step_controller.emergencyStop();
  rotate_controller.emergencyStop();
};
