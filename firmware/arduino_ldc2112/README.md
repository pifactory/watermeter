# Example Arduino code for LDC2112 based water meter sensor

Small Platformio project to start making your own firmware for LDC2112 based water meter.

## This is just a demo

You have to add WiFi logic yourself and add data collection mechanism of choice (MQTT?) to connect it to your home automation platform.

## Known Issues

### GPIO interrupts do not trigger properly

Keep in mind that LDC2112 works from 1.8V and not 3.3V. This means that you get only 1.6-ish volts on interrupt lines coming from the chip. On most ESP-based boards I have in my possession this is not enough for reliable triggering of GPIO inputs.

There are 2 solutions:

*  Use i2c registers (like in this demo)
*  Make level translator `1.8V` --> `3.3V` (and enjoy `attachInterrupt(...)`)

### Sensor becomes blind when wheel stops under the coil for too long

LDC2112 is designed for inductive keyboard and has number of algorythms to reliably detect key press. One of them - baseline tracking. So, the chip detects rapid changes of inductance of the coil. When wheel stops or is moving too slowly, sensor can become 'blind'.

Best strategies to apply:
* Stop tracking baseline when the channel is active (wheel is under the coil)
* Change the speed of baseline tracking: slow for positive values, fast for negative
* Use both coils to detect movement. Baseline is tracked independently for each of them, so the chance that both become blind at the same time is low
* Define some mitigation measures once inconsistent triggering detected (coils trigger out of natural order):
  * Read status register (0x00) - this will clear TIMEOUT flag, or any other error flags
  * Temporarily enable full baseline tracking on misbehaving coil - may be it is just way off environment changes?

I advice also logging STATUS, OUT, DATA0 and DATA1 registers (0x00 - 0x05) when debugging this issue. Might give new insights on how further improve the algorythm.

### Reading is not 100% reliable

I've got it as far as 0.1% error of the actual value read on the meter itself, this is after 6 months. By no means statistically significant, just give you an idea.

That said, water meter reading via inductive sensor is not 100% reliable by design. Wheel on the water meter can be as slow or as fast as:
* I have 20l/min capacity on my home water supply, so full turn takes at least 3 seconds
* leaking water: wheel moves as slowly as you can imagine ([Parmenides](https://en.wikipedia.org/wiki/Philosophy_of_motion))
* No movement

The latter 2 are difficult to distinguish from slowly changing environment conditions, like temperature and humidity. What sensor sees is only slow change of induction, but there is no way to see why it is changing.

Both scan rate and sensitivity should not allow for undetected turns. The challenge here is that the wheel can stop at **any** position and the coils are sensed only so often by the chip. So, we can get stroboscope effect: like there is a movement, but it is not detected, because the wheel is moving too fast through the coil sensing zone and stops in the blind zone.