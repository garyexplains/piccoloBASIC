#!/usr/bin/env python
import time
import serial

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
        x=ser.readline()
        if len(x) == 0:
            print(len(x), x, "should be zero")
            continue
        print(x)
        if "+OK PiccoloBASIC CMD Mode" in x:
            break

cmd = "ls\n"
packet = string.encode(cmd)
ser.write(packet)

# Wait to enter CMD mode
while 1:
        x=ser.readline()
        if len(x) == 0:
            continue
        print(x)