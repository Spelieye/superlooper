# superlooper
An open source programmable looping pedal based off of the BYOC super8 pedal. 

I couldn't find any open source code or projects for a looping pedal so I decided to reverse engineer/make my own. Now you, the reader of this readme file, has access to schematics, pcb layout, code, gerber files, etc. for building or modifying your own looper pedal to do whatever you want! 

## The Build 

I decided to use an Atmel ATMEGA-644V-10PU chip for this looper. Why you might ask? Isn't that chip more expensive? Why not use a PIC controller? Well, I already have an AVR Pocket Programmer and experience flashing and programming Attinys and I didn't want to get a PIC programmer, so I decided to stick with what I knew. Plus the ATMEGA isn't that much more expense and has all the I/O pins that I needed. 

I create a PCB for the project. The Gerber files are available in this repo so you can use whatever pcb service you want. I used OSH Park, it cost about $110 for a minimum of three of these boards so about $36.70/board. The purple boards they make are cool. 

![superlooper_pcb](https://github.com/Spelieye/superlooper/assets/36861919/cb1c406b-90b2-4b55-8693-5ca40a13ad70)

I used a [1032L enclosure](https://www.taydaelectronics.com/hardware/enclosures/1032l-style.html). I create a drill template that can be placed on the enclosure. Cutout along the edges keeping the whitespace between the two faces. Align the bottom of the face with all the jacks to be inbetween the screw columns, tape, wrap around to the top, tape some more and you should have the template aligned properly. I center tapped, drilled pilot holes, then used a stepper drill bit for all the holes. Note: I had to drill all the holes for the PCB mounted jacks to 12mm to have tolerance so the PCB mounted jacks all fit since hand drilling isn't super precisce. 

![IMG_7229](https://github.com/Spelieye/superlooper/assets/36861919/9c1bf084-68d5-457d-b879-7cf54a031f0c)

![IMG_7231](https://github.com/Spelieye/superlooper/assets/36861919/47ed8318-5f2b-483e-b495-c9594be5f95e)

![IMG_7232](https://github.com/Spelieye/superlooper/assets/36861919/4012924f-9591-4982-94b7-41417ad2516c)

### Populated PCB: 

![IMG_7256](https://github.com/Spelieye/superlooper/assets/36861919/0c7a2982-c127-4e15-beac-e1ac6c1f7900)

### Wiring it up: 

![IMG_7403](https://github.com/Spelieye/superlooper/assets/36861919/674914ac-2013-4e40-8c90-3b42e8665469)
*Wiring the switches*

![IMG_7404](https://github.com/Spelieye/superlooper/assets/36861919/65a4c9bf-5a02-442a-8044-7774005be9e5)
*Everything wired up and needing to be pushed down for the PCB jacks to fit into the lower holes (top holes, depending on orientation)*

For the LEDs, put them through the bottom of the PCB and bend the long leg a bit to keep them from falling out when the PCB is mounted into the enclosure. Green on top row and red on the bottom row. Once the PCB is mounted you can work the LEDs into their position, either into a hole or into an [LED holder](https://www.taydaelectronics.com/5mm-led-lampshade-protector-clear.html) that I used. It's a pain in the ass to get them all in so make sure the LEDs work before you solder them into place.

Note: The silkscreen on the prototype PCB doesn't match the actual placement of the red and green LEDs. They are switched, this was an accident when I was creating the PCB but I actually like having the red loop leds closer to the switches. Happy accident. I've updated the silkscreen in the PCB files to show the correct placement of the LEDs. 

![IMG_7405](https://github.com/Spelieye/superlooper/assets/36861919/f394a476-8ef0-43cc-a6b2-af65d8d72af1)

![IMG_7407](https://github.com/Spelieye/superlooper/assets/36861919/54bffe02-49e9-4279-8825-9da0302b45ce)

### Final Assembly:

![IMG_7419](https://github.com/Spelieye/superlooper/assets/36861919/645fad5f-1e43-41ef-8fdf-69f4468e6afd)

![IMG_7417](https://github.com/Spelieye/superlooper/assets/36861919/0ff50cdf-7e6e-4f75-bba3-27e7be6dcc22)
*Switches have been labled. 1 - 5 right to left*

## Programming

The addition of ISP pins on the PCB makes flashing the chip easy, no need to remove the chip from the socket anytime you want to change something, amazing! I used a [Pocket AVR Programmer](https://www.sparkfun.com/products/9825), `avr-dude`, and a compile script which relies on `avr-gcc`. 

![IMG_7415](https://github.com/Spelieye/superlooper/assets/36861919/621d9997-f6f5-4bb7-ab10-bca4c11a2411)
*AVR Programmer connected to the ISP port*

To flash the chip do the following in a terminal: 
1. Cd into the directory that contains `superlooper.c` and `compile_script`
2. `./compile_script` this will create a `.hex` file that can be flashed to the chip
3. The first prompt of the script will ask what MCU you are going to use, put `atmega644` and type `superlooper` into the next prompt. Here is the cli:
 ```
What is your target MCU? (Default atmega328p):  atmega644
Name of .c program without suffix (ex: led): superlooper
Compiling superlooper.c to superlooper.hex:

# cc1 0.08 0.02
# as 0.00 0.00
# collect2 0.01 0.01

AVR Memory Usage
----------------
Device: atmega644

Program:    3192 bytes (4.9% Full)
(.text + .data + .bootloader)

Data:         50 bytes (1.2% Full)
(.data + .bss + .noinit)


Done!
```
4. Use `avr-dude` to change the chip fuses, which needs to be done to undo JTAG programming to allow some of the I/O pins to be used as outputs, and to flash the `superlooper.hex` file. 
```
avrdude -c usbtiny -p m644 -U lfuse:w:0x62:m -U hfuse:w:0xd9:m -U efuse:w:0xff:m -U flash:w:superlooper.hex
```
The fuses only need to be changed once so to update the firmware if you make any changes you only need to run:
```
avrdude -c usbtiny -p m644 -U flash:w:superlooper.hex
```

Here is a great [AVR Fuse Calculator](https://www.engbedded.com/fusecalc/) if you are curious. 

5. Enjoy looping madness!

![IMG_7414](https://github.com/Spelieye/superlooper/assets/36861919/59a3f8a6-223e-44ff-8f30-d4d07578a6f6)
*Looper in the preset mode* 

## Features 

This looper was programmed to have the same features as the BYOC super8 with a couple minor tweaks. Here's a video for how to use the looper: [How to use the pedal](https://www.youtube.com/watch?v=KKhdrYQu3Yc)

Here are the changes that I made: 
1. To change mode press switch 1, 2, and 3
2. To change bank press switch 3, 4, and 5
3. If you want to save a preset with all the loops on, you can do that once per bank. You're welcome crazy person.
4. There are two startup led sequences I programmed that happen upon powering on the pedal. You can comment those out in the code if you don't want them but I have the sequence led function on and it's pretty cool. [Here is a video](https://youtu.be/cNmv9_9q_pk)

That's about it, I hope you enjoy!

### Power Requirements 

* 9V @ 260mA
  









