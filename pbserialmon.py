#!/usr/bin/env python
import time
import serial
import sys
import os

def waitforok(serl):
    while 1:
        r=serl.readline()
        rstr = str(r, 'ascii')
        if len(rstr) == 0:
            continue
        if "+OK" in rstr:
            break

def serialmon():
    ser = serial.Serial(
            port=sys.argv[1],
            baudrate = 9600,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            bytesize=serial.EIGHTBITS,
            timeout=1
    )

    while 1:
        r=ser.readline()
        rstr = str(r, 'ascii')
        if len(rstr) == 0:
            continue
        print(rstr, end="")

def uploader():
    print("Uploading " + sys.argv[1] + " to piccoloBASIC via " + sys.argv[2])
    file_stats = os.stat(sys.argv[1])
    if file_stats.st_size < 0:
        print("Can't get file size of " + sys.argv[1])
        exit(1)

    ser = serial.Serial(
            port=sys.argv[2],
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
    print("Starting upload of " + sys.argv[1] + " (" + str(file_stats.st_size) + " bytes)")

    cmd = "upload " + sys.argv[1] + " " + str(file_stats.st_size) + "\n"
    packet = bytearray(cmd, 'ascii')
    ser.write(packet)

    waitforok(ser)

    with open(sys.argv[1], 'rb') as f:
        byte = f.read(1)
        while byte != b'':
            up = "" + str(ord(byte)) + "\n"
            #print(up)
            packet = bytearray(up, 'ascii')
            ser.write(packet)
            waitforok(ser)
            byte = f.read(1)

    cmd = "exit\n"
    packet = bytearray(cmd, 'ascii')
    ser.write(packet)

    print("Upload complete.")
    exit(0)

print("PiccoloBASIC serial monitor and file uploader.")

if len(sys.argv)==2:
    serialmon()

if len(sys.argv)==3:
    uploader()

print("Usage: " + sys.argv[0] + "[<filename>] <device>");
print("If you include a file, as the first parameter, it will be uploaded to the board running PiccoloBASIC.")
print("eg: " + sys.argv[0] + " /dev/ttyACM0");
print("eg: " + sys.argv[0] + " main.bas /dev/ttyACM0");

exit(1)

