# led_clock

- LED Clock is a project I worked on to reuse a 4 digit 7-segment LED display scavenged from an old alarm clock.
- Arduino INO included used with Teensy 3.1 to drive the LED display as a clock.  
- Added also mode to support counting up.

## LED Reverse Engineering
I wasn't given the directions on how to hook up the LED display.  Through trial an error using one wire connected to 3V and one to ground, was able to figure out how the display worked.   Generally most LED displays are either common cathode or common anode.  That's the first part to figure out.  The best way to start was to cut the LED display completely from the alarm clock board, as the board itself might be tying some things to ground or shorting them together that may interfere with determining the LED segment wiring.

In addition, I was able to find the spec for the chip in the alarm clock that serves as the 'brains' of the alarm clock, the LM8560 alarm clock driver chip.  Inspecting how the LED display was wired to the pins on this chip (following PCB traces) helped also further determine not only the wiring but a strategy for driving the display more efficiently than I did initially.

- [See Google sheet I used in figuring out LED segment connections](https://docs.google.com/spreadsheets/d/1uvN9qPjFpo4JUEpYhtVIMktASHFhyrPmAeeo7BXrQFY/edit?usp=sharing)
- [See spec for UTM LM8560 alarm clock chip](http://www.paulanders.com/G5-LED/ver1/datablad.pdf)

## To Do
- Add shift register to permit using fewer GPIO pins and increase drive current and voltage to LEDs
- Add RTC (real time clock) functionality so circuit can more accurately measure time
- Had thought about adding buttons and switches to permit setting the time, setting an alarm, choosing the mode, etc. but that's so last century... instead will add bluetooth serial capability so can interface the Teensy to an Android app so you can set the time.
- Another possibility is to have wifi capability on the clock so the time can be set via the internet in some way but that may require a different driver chip that has built-in wifi instead.
- Add countdown timer based both on starting time (count to 0) and time left until a specific time (delta from future time)

## Coding Strategy
The code strategy is basically this:
- The LED display is common cathode with basically 2 cathodes (there are 3 but it only serves to drive one LED segment, having an independent anode so can drive its cathode along with one of the others without issue).
- In the main loop we keep a cycle count.  We only care whether it is cycle 0 or 1 so just have it alternate between 0 and 1 value.
- Depending on cycle, we drive cathodes and anodes per the digits we want to display.  In any one cycle we don't drive all necessary segments, since if we did we might accidentally turn on undesired segments due to the common anode connectivity, so we have to alternate in time between sets of LEDs we want to drive.  This is the essense of most every LED display since we can take advantage of how slow our eyes and brain perceive images and how fast the LEDs and microprocessor can be driven in order to reduce the amount of wiring necessary in the display.   In this particular LED display the savings isn't great but imagine having a wire for each anode or cathode of each and every LED pixel in a 4K RGB display.  At a minimum that would be over 20 million wires!!
- We control the refresh rate of the display by basically controlling the delay at the end of the main loop.
- We calculate current seconds elapsed since device has been booted.
- We hard-code (for now) the starting time in minutes and hours and calculate the current time from that.  It is easier to calculate the time intially in 24-hr format then go to 12hr format.  With this particular display we have no choice to go with 12hr format as the first digit only has 2 LED segments connected to the external pins.  (The other segments in that digit appear to be there but no traces go from them to the main PCB under the display.  If I really wanted to, I could probably access them, but it may end up destroying the board and not really be worth it!).
- Added debug option to support accelerated time so we can check that 'roll overs' occur properly-- added FASTFACTOR in code to server to multiply the milliseconds elapsed so we don't truncate seconds too quickly.
- Added debug option to have some stuff go to serial port if desired to check on calculations.
- Added a new mode to support calculating up from 0 and show minutes and seconds instead.

