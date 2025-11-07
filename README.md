# DiGee MCMD

## Introduction
The DiGee MCMD is a polyrhythmic midi clock device that allows you to play two midi sequenced devices in synchronisation, one device as a constant 'master clock' and the second as a polyrhythmic rhythm of the master clock.

The polyrhythm is calculated by the user building a fraction using binary digits to tally a number between 1 and 16, either using this number as the numerator over 16, or as the denominator under 16.

This fraction becomes the ratio for the polyrhythmic bpm to the master, and can be updated live.

## Details

Configured for Teensy 4.0. with Teensyduino libraries such as IntervalTimer, additional libraries are included in the repo for clarity but installation via the Arduino IDE is recommended.
https://www.pjrc.com/teensy/tutorial.html 
