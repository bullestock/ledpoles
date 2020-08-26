#!/usr/bin/env python3

import socket, time, sys

if len(sys.argv) < 2:
    print("Usage: setmode [<address>] <mode>")
    sys.exit()

address = 'displaydingo1.local'
mode = sys.argv[1]

if len(sys.argv) > 2:
    address = sys.argv[1]
    mode = sys.argv[2]
    
print("Setting mode %s for %s" % (mode, address))

PORTNUM = 7890

s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, 0)
s.connect((address, PORTNUM))

data = '5804'

data = bytes.fromhex(data)+mode.encode('utf-8')

s.send(data)

s.close()
