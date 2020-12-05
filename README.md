# TreadmillController
Raspberry pi server software for use in controlling the speed of a treadmill remotely.

## Requirements
- Any treadmill with a controller board model MC2100L
- Raspberry Pi with a PWM signal, tested with Raspberry Pi 3
## Setup
There is a header cable connected to the MC2100L board from the user controller (at the top of the treadmill). This must be altered in the following way. The blue wire must be connected to the Raspberry Pi's Positive PWM terminal, while the ground tied to the negative terminal.
![](images/diag.png)
The cable looks like this on our device.
![](images/cable.png)
