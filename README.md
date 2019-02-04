# Mbot Joy

I had an [arcade joystick][] sitting around and I thought it would be fun to hook it up to my [mbot][] (an introductory robotics platform which is basically an arduino uno with an integrated motor controller). The code in this project allows you to do just that: connect a joystick and drive your mbot around. As an added bonus, it uses the ultrasonic sensor to avoid running into obstacles. It will slow down as it approaches an obstacle, and stop when it gets too close.

This code is designed to be compiled with `avr-gcc`.  It will **not** work with the Arduino IDE.

## Connections

A standard arcade joystick has five connections (a ground pin and then one pin for each direction).  I soldered a header behind ports 1 and 2, which provides access to digital pins 9, 10, 11, and 12. You could also use the header behind ports 3 and 4 (and then modify the code appropriately).  Alternatively, you could grab a pair of [RJ25 to dupont adapters][] and not have to solder anything.

[arcade joystick]: https://arcadeshock.com/products/seimitsu-ls-32-01-joystick
[mbot]: https://www.makeblock.com/steam-kits/mbot
[rj25 to dupont adapters]: https://store.makeblock.com/rj25-to-dupont-wire-pair

## See also

The Doxygen docs for this code are online at <http://oddbit.com/avr-mbot-joy/>.
