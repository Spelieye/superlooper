# superlooper
An open source programmable looping pedal based off of the BYOC super8 pedal. 

I couldn't find any open source code or project for a looping pedal so I decided to reverse engineer/make my own. Now you reader of this readme file have access to schematics, pcb layout, code, gerber files, etc. for building or modifying your own looper pedal to do whatever you want! 

## The Build 

I decided to use an Atmel ATMEGA-644V-10PU chip for this looper. Why you might ask? Isn't that chip more expensive? Why not use a PIC controller? Well, I already have an AVR Pocket Programmer and experience flashing and programming Attinys and I didn't want to get a PIC programmer, so I decided to stick with what I knew. Plus the ATMEGA isn't that much more expense. Get over it.



## Programming 

The addition of some ISP pins on the PCB makes updating the firmware super easy. I've also included a *compile_script* that will compile the .c file to .hex for the given target atmel MCU

1. Use an [avr pocket programmer](https://www.sparkfun.com/products/9825) (or any avr programmer of your choice)

Command line for flashing the chip:

`avrdude -c usbtiny -p m644 -U flash:w:superlooper.hex `
