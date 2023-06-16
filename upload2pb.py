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

while 1:
        x=ser.readline()
        print(x)