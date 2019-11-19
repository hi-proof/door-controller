/** Define an interface for the floor movement controller
 *
 */

#pragma once

#ifndef HI_PROOF_FLOOR_CONTROLLER_H_
#define HI_PROOF_FLOOR_CONTROLLER_H_

#include "Bounce2.h"
#include "TeensyStep.h"
#include "elevator_config.h"
#include "safety_critical_component.h"

namespace hiproof {

namespace elevator {

class FloorController final : SafetyCriticalComponent {
 public:
  static constexpr uint8_t kNumNamedStops = 4;

  FloorController(const int step_pin, const int dir_pin, const int home_pin, const int overrun_pin);
  FloorController(config::EthernetConnectorPins conn);
  ~FloorController();

  // This function is for non automatic init (actual hardware control)
  void init();

  void seekToHome();
  void moveToStop(uint8_t stop_number);
  void update();
  void stop();

  // API for "maintenance mode" tasks
  void setStop(uint8_t stop_number);
  void rotate(bool clockwise);
  void stopRotation();
  // void rotateWithStep(uint16_t steps, bool clockwise);

  void enterSafeMode();
  void emergencyStop();

 private:
  uint32_t named_stops_[kNumNamedStops];
  Stepper stepper_;
  Bounce home_pin_ ;
  Bounce overrun_pin_;
  uint16_t run_speed{config::kFloorMaxLiveSpeed};
  uint16_t run_accel{config::kFloorMaxLiveAccel};
  bool sc_active{false};
  bool rc_active{false};

  static StepControl step_controller;
  static RotateControl rotate_controller;

  void startStepController();
  void stopStepController();
  void startRotateController();
  void stopRotateController();
};

}  // namespace elevator

}  // namespace hiproof

#endif  // HI_PROOF_FLOOR_CONTROLLER_H_
