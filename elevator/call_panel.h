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
     enum {
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
     ~CallPanel();

     void setButtonCallback(ButtonPressCallback on_press,
                            ButtonHoldCallback on_hold);

    private:
     FancyButton button_map_[MAX_BUTTONS];
};

} // elevator
} // hiproof

#endif // HI_PROOF_CALL_PANEL_H_