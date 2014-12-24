import socket

UDP_IP = "192.168.1.220"
UDP_PORT = 6666
MESSAGE = "ping"

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.sendto(MESSAGE, (UDP_IP, UDP_PORT))
print "Sent ping to " + str(UDP_IP) 	