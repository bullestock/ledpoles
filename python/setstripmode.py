#!/usr/bin/env python3

import socket, time, sys

if len(sys.argv) < 2:
    print("Usage: setstripmode [<address>] <mode>")
    sys.exit()

address = 'displaydingo1.local'
mode = sys.argv[1]
if len(sys.argv) > 2:
    address = sys.argv[1]
    mode = sys.argv[2]

mode = int(mode)
print("Setting strip mode %d for %s" % (mode, address))

PORTNUM = 7890

s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, 0)
s.connect((address, PORTNUM))

d = 0.2

data = '5904%02x' % mode
    
s.send(bytes.fromhex(data))

s.close()
