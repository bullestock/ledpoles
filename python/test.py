#!/usr/bin/env python3

import socket, time, sys

address = 'displaydingo1.local'
if len(sys.argv) > 1:
    address = sys.argv[1]

PORTNUM = 7890

# initialize a socket, think of it as a cable
# SOCK_DGRAM specifies that this is UDP
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, 0)
 
# connect the socket, think of it as connecting the cable to the address location
s.connect((address, PORTNUM))

d = 0.2

while True:
    data = '5704'
    data = data + '0000' # offset
    for i in range(0, int(4*30/4)):
        data = data + '00000000ffff000000ff0000' # black, blue, black, red
    
    s.send(bytes.fromhex(data))

    time.sleep(d)
    
    data = '5704'
    for i in range(0, int(4*30/4)):
        data = data + '0000ff000000ff0000000000' # blue, black, red, black
    
    s.send(bytes.fromhex(data))

    time.sleep(d)
    
# close the socket
s.close()
