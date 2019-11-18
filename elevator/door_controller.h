/** Interface specs for a single door controller
 * 
 * @todo Will need some syncronization systems
 * 
 */

#pragma once

#ifndef HI_PROOF_DOOR_CONTROLLER_H_
#define HI_PROOF_DOOR_CONTROLLER_H_

#include "safety_critical_component.h"

namespace hiproof {
namespace elevator {

class DoorController final : SafetyCriticalComponent {};

} // elevator
} // hiproof

#endif // HI_PROOF_DOOR_CONTROLLER_H_