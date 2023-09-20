# superlooper
An open source programmable looping pedal based off of the BYOC super8 pedal. 

I couldn't find any open source code or project for a looping pedal so I decided to reverse engineer/make my own. Now you reader of this readme file have access to schematics, pcb layout, code, gerber files, etc. for building or modifying your own looper pedal to do whatever you want! 

## The Build 

I decided to use an Atmel ATMEGA-644V-10PU chip for this looper. Why you might ask? Isn't that chip more expensive? Why not use a PIC controller? Well, I already have an AVR Pocket Programmer and experience flashing and programming Attinys and I didn't want to get a PIC programmer, so I decided to stick with what I knew. Plus the ATMEGA isn't that much more expense. Get over it.

## Programming

The addition of ISP pins on the PCB makes flashing the chip easy, no need to remove the chip from the socket anytime you want to change something, amazing! 
