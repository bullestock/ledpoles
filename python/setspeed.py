import socket, time, sys

if len(sys.argv) != 2:
    print("Usage: setspeed <mode>")
    sys.exit()

speed = int(sys.argv[1])
print("Setting speed %d" % speed)
# addressing information of target
IPADDR = 'displaydingo1.local'
PORTNUM = 7890

s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, 0)
s.connect((IPADDR, PORTNUM))

d = 0.2

data = '5A04%02x' % speed
    
s.send(data.decode('hex'))

s.close()
