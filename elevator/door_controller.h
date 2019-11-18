/** Interface specs for a single door controller
 *
 * @todo Will need some syncronization systems
 * @todo realistically most of this code is the same as the floor, but like.. eh..
 *
 */

#pragma once

#ifndef HI_PROOF_DOOR_CONTROLLER_H_
#define HI_PROOF_DOOR_CONTROLLER_H_

#include "safety_critical_component.h"

namespace hiproof {
namespace elevator {

class DoorController final : SafetyCriticalComponent {
 public:
  DoorController(const int step_pin, const int dir_pin, const int home_pin,
                 const int overrun_pin);
  ~DoorController(){};
  void enterSafeMode();
  void emergencyStop();

 private:
};

}  // namespace elevator
}  // namespace hiproof

#endif  // HI_PROOF_DOOR_CONTROLLER_H_