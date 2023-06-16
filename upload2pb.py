#!/usr/bin/env python
import time
import serial
import sys

if len(sys.argv)!=3:
    print("Usage: " + sys.argv[0] + "<filename> <device>");
    print("eg: " + sys.argv[0] + " main.bas /dev/ttyACM0");

ser = serial.Serial(
        port='/dev/ttyACM0',
        baudrate = 9600,
        parity=serial.PARITY_NONE,
        stopbits=serial.STOPBITS_ONE,
        bytesize=serial.EIGHTBITS,
        timeout=1
)

packet = bytearray()
packet.append(0x03)
packet.append(0x03)

ser.write(packet)

# Wait to enter CMD mode
while 1:
        r=ser.readline()
        rstr = str(r, 'ascii')
        if len(rstr) == 0:
            continue
        print(r, rstr)
        if "+OK PiccoloBASIC CMD Mode" in rstr:
            break
print("Entered CMD mode.")
cmd = "ls\n"
packet = bytearray(cmd, 'ascii')
ser.write(packet)

# Wait to enter CMD mode
while 1:
        x=ser.readline()
        r=ser.readline()
        rstr = str(r)
        if len(rstr) == 0:
            continue
        print(x)