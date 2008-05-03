# See if we can do multicasting in Python
import socket
import struct
import time
HOST = ''
PORT = 18555
s = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
s.bind((HOST,PORT))
print "Cool"
# mcast addr, local interface, if index (should be 0)
mcast_addr = socket.inet_aton('224.0.0.142')
# mcast_addr = socket.inet_aton('1.0.0.1')
any_addr = socket.inet_aton('0.0.0.0')
ipmreqn = mcast_addr + any_addr + struct.pack("i", 0)
print repr(mcast_addr)
print repr(any_addr)
print repr(ipmreqn)
s.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP,
	ipmreqn)
s.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, 1)
print "Excellent"
time.sleep(90)

