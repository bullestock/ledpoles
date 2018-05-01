import socket, time, sys

if len(sys.argv) < 2:
    print("Usage: setspeed [<address>] <speed>")
    sys.exit()

address = 'displaydingo1.local'

speed = sys.argv[1]
if len(sys.argv) > 2:
    address = sys.argv[1]
    speed = sys.argv[2]

speed = int(speed)
print("Setting speed %d for %s" % (speed, address))

PORTNUM = 7890

s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, 0)
s.connect((address, PORTNUM))

d = 0.2

data = '5A04%02x' % speed
    
s.send(data.decode('hex'))

s.close()
