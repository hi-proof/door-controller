/** Define an interface for some basic safety sensitive devices
 * 
 */

#pragma once

#ifndef HI_PROOF_SAFETY_CRITICAL_COMPONENT_H_
#define HI_PROOF_SAFETY_CRITICAL_COMPONENT_H_

namespace hiproof {

namespace elevator {

class SafetyCriticalComponent {
 public:
  virtual ~SafetyCriticalComponent();
  virtual void enterSafeMode() = 0;
  virtual void emergencyStop() = 0;

  bool isSafe() { return safe_; }

 protected:
  bool safe_;
};

}  // elevator

}  // hiproof

#endif  // HI_PROOF_FLOOR_CONTROLLER_H_
