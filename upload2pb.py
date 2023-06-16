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
        r=ser.readline()
        rstr = str(r)
        if len(rstr) == 0:
            print(len(rstr), rstr, "should be zero")
            continue
        print(x, rstr)
        if "+OK PiccoloBASIC CMD Mode" in rstr:
            break

cmd = "ls\n"
packet = string.encode(cmd)
ser.write(packet)

# Wait to enter CMD mode
while 1:
        x=ser.readline()
        r=ser.readline()
        rstr = str(r)
        if len(rstr) == 0:
            continue
        print(x)