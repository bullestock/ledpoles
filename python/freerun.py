#!/usr/bin/env python3

import socket, time, sys

if len(sys.argv) < 2:
    print("Usage: freerun [<address>]")
    sys.exit()

address = 'displaydingo1.local'

if len(sys.argv) > 1:
    address = sys.argv[1]
    
print("Setting free run mode for %s" % address)

PORTNUM = 7890

s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, 0)
s.connect((address, PORTNUM))

data = '5B04'
    
s.send(bytes.fromhex(data))

s.close()
