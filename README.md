# airmouse
control your computer mouse with your hand, wirelessly
#### v26.0.0

## parts you will need (will change with updates)
- ESP32
- L3GD20H 3-Axis Gyroscope
- 4x Contact Pads (or smth else conductive to put/tape on your fingers)
- other wires/breadboard


## how to make
- plug the accelerometer into esp32 sda, scl, 3v3, and ground
- plug whatever you are using for contact detection into ground and their respective pins
  - my finger preference is movement unlock for pointer, left click middle, right click ring
- close the contact detection circuit to be able to move the mouse, and take a guess what the other 2 do

## build instructions
this is a platformio project, so you need that installed, probably easiest on VS Code. To build and open serial monitor run with <code>pio</code>.
```
pio run -t upload -t monitor
```
it should auto detect the proper port