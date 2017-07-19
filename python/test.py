import socket, time

# addressing information of target
IPADDR = '192.168.0.58'
PORTNUM = 7890

# initialize a socket, think of it as a cable
# SOCK_DGRAM specifies that this is UDP
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, 0)
 
# connect the socket, think of it as connecting the cable to the address location
s.connect((IPADDR, PORTNUM))

d = 0.2

while True:
    data = '570400000000ffff000000ff000000ffff000000ff000000ffff000000ff000000ffff000000ff000000ffff000000ff000000ffff000000ff000000ffff000000ff000000ffff000000ff000000ffff000000ff000000ffff000000ff000000ffff000000ff000000ffff000000ff000000ffff000000ff000000ffff000000ff000000ffff000000ff000000ffff000000ff000000ffff000000ff000000ffff000000ff000000ffff000000ff000000ffff000000ff000000ffff000000ff000000ffff000000ff00'
    
    s.send(data.decode('hex'))

    time.sleep(d)
    
    data = '57040000ff000000ff000000ffff000000ff000000ffff000000ff000000ffff000000ff000000ffff000000ff000000ffff000000ff000000ffff000000ff000000ffff000000ff000000ffff000000ff000000ffff000000ff000000ffff000000ff000000ffff000000ff000000ffff000000ff000000ffff000000ff000000ffff000000ff000000ffff000000ff000000ffff000000ff000000ffff000000ff000000ffff000000ff000000ffff000000ff000000ffff000000ff000000ffff000000ff000000ff'
    
    s.send(data.decode('hex'))

    time.sleep(d)
    
# close the socket
s.close()
