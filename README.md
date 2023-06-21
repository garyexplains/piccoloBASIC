# piccoloBASIC
A BASIC interpreter for the Raspberry Pi Pico (i.e. BASIC for microcontrollers)

## Videos
I have lots of information about PiccoloBASIC in several videos on my YouTube channel [Gary Explains](https://youtube.com/c/GaryExplains)

Watching these videos is the quickest way to understand what this project is about.

- [I wrote a BASIC interpreter! What should I do with it?](https://youtu.be/4MiT-29I_jI)
- [My BASIC interpreter for the Raspberry Pi Pico is working!](https://youtube.com/shorts/7dQfG__jr_c)

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

## Usage
Flash `piccoloBASIC.uf2` on to your Pico
Use 

## History
The starting point for piccoloBASIC was "uBASIC: a really simple BASIC interpreter" by Adam Dunkels.

- http://dunkels.com/adam/ubasic/
- https://github.com/adamdunkels/ubasic

Here is what Adam said about the project:

> Written in a couple of hours, for the fun of it. Ended up being used in a bunch of places!

> The (non-interactive) uBASIC interpreter supports only the most basic BASIC functionality: if/then/else, for/next, let, goto, gosub, print, and mathematical expressions. There is only support for integer variables and the variables can only have single character names. I have added an API that allows for the program that uses the uBASIC interpreter to get and set BASIC variables, so it might be possible to actually use the uBASIC code for something useful (e.g. a small scripting language for an application that has to be really small).


Modifications by Gary Sims
==========================

Building on the excellent work of Adam Dunkels, I have tweaked this for my needs.

### Changes
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

### Working on

### BUGS
- Many!
- Floating point literals don't work in "if" statements: if b# < 20.9 then print "Boom"
- printf is seen as print by the tokenizer as the first 5 letters are the same
- There is probably a memory leak somewhere related to strings.

## Contributing
There is lots to do! Please feel free to fork and/or continue working on Piccolo BASIC as you see fit.

## Resources

## License - 3-Clause BSD License
Copyright (C) 2006, Adam Dunkels

Copyright (C) 2023, Gary Sims

All rights reserved.

SPDX short identifier: BSD-3-Clause
