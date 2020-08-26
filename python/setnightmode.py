#!/usr/bin/env python3

import socket, time, sys

if len(sys.argv) < 2:
    print("Usage: setnightmode [<address>] <mode>")
    sys.exit()

address = 'displaydingo1.local'

nightmode = sys.argv[1]
if len(sys.argv) > 2:
    address = sys.argv[1]
    nightmode = sys.argv[2]

nightmode = int(nightmode)
print("Setting night mode %d for %s" % (nightmode, address))

PORTNUM = 7890

s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, 0)
s.connect((address, PORTNUM))

d = 0.2

data = '5D04%02x' % nightmode
    
s.send(bytes.fromhex(data))

s.close()
