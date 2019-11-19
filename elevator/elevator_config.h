/** Configuration options and constants for the elevator
 * 
 */

#ifndef HI_PROOF_ELEVATOR_CONFIG_H_
#define HI_PROOF_ELEVATOR_CONFIG_H_

namespace hiproof {
namespace elevator {
namespace config {

constexpr bool kSystemDebugCLI = true;

using EthernetConnectorPins = struct EthernetConnectorPin_t {
  int step_pin;
  int dir_pin;
  int home_pin;
  int overrun_pin;
};

constexpr EthernetConnectorPins kEthPortA = {
    .step_pin = 34, .dir_pin = 33, .home_pin = 35, .overrun_pin = 36};

constexpr EthernetConnectorPins kEthPortB = {
    .step_pin = 38, .dir_pin = 37, .home_pin = 39, .overrun_pin = 32};

constexpr EthernetConnectorPins kEthPortC = {
    .step_pin = 30, .dir_pin = 31, .home_pin = 28, .overrun_pin = 29};

constexpr EthernetConnectorPins kEthPortD = {
    .step_pin = 19, .dir_pin = 18, .home_pin = 11, .overrun_pin = 9};

static constexpr uint16_t kFloorHomingSpeed = 1000;
static constexpr uint16_t kFloorHomingAccel = 1000;
static constexpr uint16_t kFloorMaxLiveSpeed = 1500;
static constexpr uint16_t kFloorMaxLiveAccel = 500;

static constexpr uint16_t kDoorHomingSpeed = 400;
static constexpr uint16_t kDoorHomingAccel = 1000;
static constexpr uint16_t kDoorMaxLiveSpeed = 5000;
static constexpr uint16_t kDoorMaxLiveAccel = 1500;

static constexpr uint16_t kPinDebounceInterval = 25;

}
}
} // namespace hiproof

#endif // HI_PROOF_ELEVATOR_CONFIG_H_
