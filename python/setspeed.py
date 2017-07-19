import socket, time, sys

if len(sys.argv) != 2:
    print("Usage: setspeed <mode>")
    sys.exit()

speed = int(sys.argv[1])
print("Setting speed %d" % speed)
# addressing information of target
IPADDR = '192.168.0.58'
PORTNUM = 7890

s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, 0)
s.connect((IPADDR, PORTNUM))

d = 0.2

data = '5A04%04x' % socket.htons(speed)
    
s.send(data.decode('hex'))

s.close()
