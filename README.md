# piccoloBASIC
A BASIC interpreter for the Raspberry Pi Pico (i.e. BASIC for microcontrollers)

## Videos
I have lots of information about PiccoloBASIC in several videos on my YouTube channel [Gary Explains](https://youtube.com/c/GaryExplains)

Watching these videos is the quickest way to understand what this project is about.

- [Program the Raspberry Pi Pico using BASIC - Introducing PiccoloBASIC](https://youtu.be/oWyMGDDcykY)
- [I wrote a BASIC interpreter! What should I do with it?](https://youtu.be/4MiT-29I_jI)
- [My BASIC interpreter for the Raspberry Pi Pico is working!](https://youtube.com/shorts/7dQfG__jr_c)

## Press Coverage

- [PiccoloBASIC – A BASIC interpreter for the Raspberry Pi Pico board](https://www.cnx-software.com/2023/06/23/piccolobasic-basic-interpreter-raspberry-pi-pico-board/)
- [Raspberry Pi Pico Gets Basic Interpreter Called PiccoloBASIC](https://www.tomshardware.com/news/raspberry-pi-pico-basic-interpreter-piccolobasic)

## A Comma A Day Keeps The Pedants Away
Since the Internet seems to be full of people with way too much time on their hands, I would just like to kindly shoo away any C/C++ pedants out there. To be honest, I am not interested.

Having said that, like-minded people who wish to be part of the community, to contribute and to extend Piccolo BASIC are very welcome. See __Contributing__

## Build Instructions
Make sure you have the Pico C/C++ SDK installed and working on your machine. [Getting started with Raspberry Pi Pico is
the best place to start.](https://datasheets.raspberrypi.org/pico/getting-started-with-pico.pdf)

You need to have PICO_SDK_PATH defined, e.g. `export PICO_SDK_PATH=/home/pi/pico/pico-sdk/`

Clone the code from the repository. Create a directory called `build`. Change directory into `build` and run `cmake ..`

Run `make -j4`

The resulting file `piccoloBASIC.uf2` can be flashed on your Pico in the normal way (i.e. reset will pressing `bootsel` and copy the .uf2 file to the drive).

## Releases
If you don't want to build from the source code then look in [Releases](https://github.com/garyexplains/piccoloBASIC/releases) for some pre-built binaries.

## Usage
1. Flash `piccoloBASIC.uf2` on to your Pico
2. Use `pbserialmon.py` to upload your BASIC program.
   - It must be called `main.bas`
   - e.g. `./pbserialmon.py main.bas /dev/ttyACM0`
3. Use `pbserialmon.py` to see the output from your program
   - e.g. `./pbserialmon.py /dev/ttyACM0`

## Features
- Let, if, print, for, goto, gosub
- String variables (let z$="hello")
- Floating point numbers and variables (let z#=1.234)
- Builtin functions [zero, randint, not, time]
- Sleep, delay, randomize, push & pop (for integers)
- Maths functions like cos, sin, tan, sqr, etc
- LittleFS support
- Rudimentary GPIO support

## Examples
Here are some example programs written in PiccoloBASIC.
### Hello, World!
```
loop:
print "Gary Explains"
sleep 1
goto loop:
```
### For loop
```
for i = 1 to 10
let x = randint()
print x
next i
end
```
### Gosub
```
gosub asub:
for i = 1 to 10
print i
next i
print "end"
end
asub:
print "subroutine"
return
```
### Blinky
```
pininit 25
pindirout 25
loop:
print "ON"
pinon 25
sleep 1
print "OFF"
pinoff 25
sleep 1
goto loop:
```
### 99 Bottles
```
let b = 99
let b$ = "99"
let s$ = " bottles"
for a = 1 to 99
    print b$; s$; " of soda on the wall, "; b$; s$; " of soda."
    let b = b - 1
    if b > 0 then let p$ = "one"
    if b > 0 then let b$ = b else let b$ = "no more"
    if b = 0 then let p$ = "it"
    if b = 1 then let s$ = " bottle" else let s$ = " bottles"
    print "take "; p$; " down and pass it around, "; b$; s$; " of soda on the wall."
next a
print "no more bottles of soda on the wall, no more bottles of soda."
print "go to the store and buy some more, 99 bottles of soda on the wall."
```
## History
The starting point for piccoloBASIC was "uBASIC: a really simple BASIC interpreter" by Adam Dunkels.

- http://dunkels.com/adam/ubasic/
- https://github.com/adamdunkels/ubasic

Here is what Adam said about the project:

> Written in a couple of hours, for the fun of it. Ended up being used in a bunch of places!

> The (non-interactive) uBASIC interpreter supports only the most basic BASIC functionality: if/then/else, for/next, let, goto, gosub, print, and mathematical expressions. There is only support for integer variables and the variables can only have single character names. I have added an API that allows for the program that uses the uBASIC interpreter to get and set BASIC variables, so it might be possible to actually use the uBASIC code for something useful (e.g. a small scripting language for an application that has to be really small).


### Modifications by Gary Sims
Building on the excellent work of Adam Dunkels, I have tweaked this for my needs.

- ubas - A program to run a .bas script
- Removed need for linenumbers (but expectedly broke goto/gosub)
- Added labels for goto and gosub
- Added some simple error reporting
- Added sleep and delay
- Added floating point numbers and variables (let z#=1.234)
- Added some builtin functions [zero, randint, not, time]
- Added randomize
- Added push and pop (for integers)
- Added rnd, cos, sin, tan, sqr, etc
- Added string variables (let z$="hello")
- Added os, which calls system()
- Ported to Raspberry Pi Pico
- Added support for LittleFS
- Added CMD mode
- Added serial monitor and uploader tool
- Added simple GPIO functionality

### Working on
- Too much!
### BUGS
- Many!
- Floating point literals don't work in "if" statements: if b# < 20.9 then print "Boom"
- printf is seen as print by the tokenizer as the first 5 letters are the same
- There is probably a memory leak somewhere related to strings.

## LittleFS
Arm developed a fail-safe filesystem for microcontrollers, it is called LittleFS:
- Power-loss resilience - littlefs is designed to handle random power failures. 
- Dynamic wear leveling - littlefs is designed with flash in mind, and provides wear leveling over dynamic blocks.
- Bounded RAM/ROM - littlefs is designed to work with a small amount of memory. RAM usage is strictly bounded, which means RAM consumption does not change as the filesystem grows.
- Open source - under the BSD-3-Clause license.
- Used by MicroPython / CircuitPython already

See https://github.com/littlefs-project/littlefs

### LittleFS license
3-Clause BSD License  
Copyright (c) 2022, The littlefs authors.  
Copyright (c) 2017, Arm Limited. All rights reserved.

## Flash layout
Raspberry Pi Pico has 2MB of Flash.
- Total flash 2048K
- First 640K is for firmware 
  - i.e. PiccoloBASIC or MicroPython
- Rest is for LittleFS
  - BASIC programs, Python scripts etc
- This way PiccoloBASIC is compatible with MicroPython

```
0K    -------------------------------------
      |                                    |
      |      PiccoloBASIC firmware         |
      |                                    |
640K  -------------------------------------
      |                                    |
      |            LittelFS                |
      |                .                   |
      |                .                   |
      |                .                   |
      |            main.bas                |
      |                                    |
      |                                    |
2048K -------------------------------------
```

## CMD Mode
To upload BASIC scripts and store them in LittleFS, there is a CMD mode in PiccoloBASIC. Sending `CTRL-C` twice one after the other will pause the interpreter and enter CMD mode:
- ls
- cd
- rm
- reboot
- exit
- upload

See `pbserialmon.py` for details on the protocol for the upload command

## Roadmap
### More language features
- Peek and poke
- Longer variable names (currently just one letter!)
- Negative numbers + 64 bit numbers + hex numbers
- Better loops (steps, reverse, while etc)
- File IO
### More hardware support
- Complete GPIO, I2C, SPI, PIO, etc
- Networking and Bluetooth
- Support some standard displays
- USB keyboard
### PiccoloOS
Build PiccoloBASIC on top of [PiccoloOS](https://github.com/garyexplains/piccolo_os_v1.1)
- Multi-tasking BASIC 👀
- Locks and synchronization
- Dual CPU support

## Contributing
There is lots to do! Please feel free to fork and/or continue working on Piccolo BASIC as you see fit.

## Resources
- [Gary Explains](https://youtube.com/c/GaryExplains)
- [PiccoloOS](https://github.com/garyexplains/piccolo_os_v1.1)
- [LittleFS](https://github.com/littlefs-project/littlefs)
- [uBasic](https://github.com/adamdunkels/ubasic)

## License - 3-Clause BSD License
Copyright (C) 2006, Adam Dunkels  
Copyright (C) 2023, Gary Sims  
All rights reserved.  

SPDX short identifier: BSD-3-Clause
