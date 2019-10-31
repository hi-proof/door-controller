# Teensy 3.6 pinout

(see pinout.png)

* Red - b2b / serial (2 of the pins need to be serial TX/RX)
* Blue - LED ws2812 (pins need to be unused serial TX, should be limited to serial 1, 2, or 3)
* Pink - Audio - see audio shield docs. these can't be changed
* Orange - PWM
* Green - Door controllers
* Aqua - IO Panel

Notes:

1. 4 pins overlap between audio & door controller - those are for door controller D which can't be used at the same time as audio shield

2. Pin 15 is free and can be used for board id pull up / pull down
