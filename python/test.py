import socket, time

# addressing information of target
IPADDR = 'displaydingo1.local'
PORTNUM = 7890

# initialize a socket, think of it as a cable
# SOCK_DGRAM specifies that this is UDP
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, 0)
 
# connect the socket, think of it as connecting the cable to the address location
s.connect((IPADDR, PORTNUM))

d = 0.2

while True:
    data = '5704'
    for i in range(0, 4*30/4):
        data = data + '00000000ffff000000ff0000' # black, blue, black, red
    
    s.send(data.decode('hex'))

    time.sleep(d)
    
    data = '5704'
    for i in range(0, 4*30/4):
        data = data + '0000ff000000ff0000000000' # blue, black, red, black
    
    s.send(data.decode('hex'))

    time.sleep(d)
    
# close the socket
s.close()
