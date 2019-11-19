/** Interface specs for a single door controller
 *
 * @todo Will need some syncronization systems
 * @todo realistically most of this code is the same as the floor, but like..
 * eh..
 *
 */

#pragma once

#ifndef HI_PROOF_DOOR_CONTROLLER_H_
#define HI_PROOF_DOOR_CONTROLLER_H_

#include "Bounce2.h"
#include "TeensyStep.h"
#include "elevator_config.h"
#include "safety_critical_component.h"

namespace hiproof {
namespace elevator {

class DoorController final : SafetyCriticalComponent {
 public:
  DoorController(const int d1_step_pin, const int d1_dir_pin,
                 const int d1_home_pin, const int d1_overrun_pin,
                 const int d2_step_pin, const int d2_dir_pin,
                 const int d2_home_pin, const int d2_overrun_pin);
  DoorController(config::EthernetConnectorPins left,
                 config::EthernetConnectorPins right);
  ~DoorController(){};
  void enterSafeMode();
  void emergencyStop();

  void open();
  void close();
  void update();
  void stopAsync();
  void homingRoutine();

  // Debug seperation
  void openLeft();
  void closeLeft();
  void homeLeft();

  void openRight();
  void closeRight();
  void homeRight();

 private:

  using DoorData = struct {
    Stepper stepper;
    Bounce home_pin;
    Bounce overrun_pin;
    uint16_t max_travel;
  };

  DoorData left_door_;
  DoorData right_door_;
  uint16_t run_speed_{config::kDoorMaxLiveSpeed};
  uint16_t run_accel_{config::kDoorMaxLiveAccel};

  static StepControl step_controller;
  static RotateControl rotate_controller;
};

}  // namespace elevator
}  // namespace hiproof

#endif  // HI_PROOF_DOOR_CONTROLLER_H_