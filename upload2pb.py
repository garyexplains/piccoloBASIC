#!/usr/bin/env python
import time
import serial
import sys
import os

print("PiccoloBASIC uploader.")

if len(sys.argv)!=3:
    print("Usage: " + sys.argv[0] + "<filename> <device>");
    print("eg: " + sys.argv[0] + " main.bas /dev/ttyACM0");
    exit(1)

print("Uploading " + sys.argv[1] + " to piccoloBASIC via " + sys.argv[2])
file_stats = os.stat(sys.argv[1])
if file_stats.st_size < 0:
    print("Can't get file size of " + sys.argv[1])
    exit(1)

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
        if "+OK PiccoloBASIC CMD Mode" in rstr:
            break
print("Entered CMD mode.")
    
cmd = "upload " + sys.argv[1] + " " + str(file_stats.st_size) + "\n"
packet = bytearray(cmd, 'ascii')
ser.write(packet)

# Wait to enter CMD mode
while 1:
        r=ser.readline()
        rstr = str(r)
        if len(rstr) == 0:
            continue
        print(rstr)