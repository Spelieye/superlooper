# superlooper
An open source programmable looping pedal based off of the BYOC super8 pedal. 

I couldn't find any open source code or projects for a looping pedal so I decided to reverse engineer/make my own. Now you, the reader of this readme file, has access to schematics, pcb layout, code, gerber files, etc. for building or modifying your own looper pedal to do whatever you want or exaclty how I designed it! 

## The Build 

I decided to use an Atmel ATMEGA-644V-10PU chip for this looper. Why you might ask? Isn't that chip more expensive? Why not use a PIC controller? Well, I already have an AVR Pocket Programmer and experience flashing and programming Attinys and I didn't want to get a PIC programmer, so I decided to stick with what I knew. Plus the ATMEGA isn't that much more expense and has all the I/O pins that I needed. 

I create a PCB for the project. The Gerber files are available in this repo so you can use whatever pcb service you want. I used OSH Park, it cost about $110 for a minimum of three boards so about $36.70/board. The purple boards they make are cool. 

![superlooper_pcb](https://github.com/Spelieye/superlooper/assets/36861919/1f62ba5a-4f87-4298-baff-512ab6a072ce)

I used a [1032L enclosure](https://www.taydaelectronics.com/hardware/enclosures/1032l-style.html). I create a drill template that can be placed on the enclosure. Cutout along the edges keeping the whitespace between the two faces. Align the bottom of the face with all the jacks to be inbetween the screw columns, tape, wrap around to the top, tape some more and you should have the template aligned properly. I center tapped, drilled pilot holes, then used a stepper drill bit for all the holes. Note: I had to drill all the holes for the PCB mounted jacks to 12mm to add tolerance so the PCB mounted jacks all fit since hand drilling isn't super precisce. 

![IMG_7229](https://github.com/Spelieye/superlooper/assets/36861919/99b4512d-16dc-49eb-b74e-eacef47a6297)

![IMG_7231](https://github.com/Spelieye/superlooper/assets/36861919/2387f791-5252-4d6d-b9d5-d02895788efb)

![IMG_7232](https://github.com/Spelieye/superlooper/assets/36861919/625e191c-4c72-43d2-b1eb-c74cabf8c1fe)


## Programming

The addition of ISP pins on the PCB makes flashing the chip easy, no need to remove the chip from the socket anytime you want to change something, amazing! 
