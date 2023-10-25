# superlooper
An open source programmable looping pedal based off of the BYOC super8 pedal. 

I couldn't find any open source code or projects for a looping pedal so I decided to reverse engineer/make my own. Now you, the reader of this readme file, has access to schematics, pcb layout, code, gerber files, etc. for building or modifying your own looper pedal to do whatever you want or exaclty how I designed it! 

## The Build 

I decided to use an Atmel ATMEGA-644V-10PU chip for this looper. Why you might ask? Isn't that chip more expensive? Why not use a PIC controller? Well, I already have an AVR Pocket Programmer and experience flashing and programming Attinys and I didn't want to get a PIC programmer, so I decided to stick with what I knew. Plus the ATMEGA isn't that much more expense and has all the I/O pins that I needed. 

I used a [1032L enclosure](https://www.taydaelectronics.com/hardware/enclosures/1032l-style.html). I create a drill template that can be placed on the enclosure. I used a center tap, drilled pilot holes, then a stepper drill bit. 

I create a PCB for the project. The Gerber files are available in this repo so you can use whatever pcb service you want. I used OSH Park, it cost about $110 for a minimum of three boards so about $36.70/board. The purple boards they make are cool. 

![superlooper_pcb](https://github.com/Spelieye/superlooper/assets/36861919/1f62ba5a-4f87-4298-baff-512ab6a072ce)

## Programming

The addition of ISP pins on the PCB makes flashing the chip easy, no need to remove the chip from the socket anytime you want to change something, amazing! 
