import socket
import struct
import time
import select
import os

MULTICAST_GROUP_ADDR="239.255.42.42"
UDP_PORT=10042
MAX_PACKET=2048
MAGIC_BYTES="FCst"
TICK_TIME=3.0 # seconds
MAX_NODE_AGE=12.0 # seconds

class NodeInfo:
	def __init__(self):
		self.nodeid="not set yet"
		self.lastseen=0 # epoch
		self.addr='0.0.0.0'
		self.isme=0
		self.up=1

class NodeMetadata:
	# nodeid
	def __init__(self):
		self.nodeid="not set yet"
		self.nodesById = { }
		self.nodesByAddr = { }
		pass
	def startup(self):
		# Load the node id from a file...
		try:
			f = open("nodeid.dat")
			self.nodeid = f.read()
			f.close
		except IOError:
			self.newid()
		# Chop the \n off the nodeid, if present
		if (self.nodeid[-1] == "\n"):
			self.nodeid = self.nodeid[0:-1]

		print "Node ID=[%s]" % self.nodeid
		# Add our own node.
		mynode = NodeInfo()
		mynode.isme=1
		mynode.nodeid=self.nodeid
		self.addNode(mynode)
		
	def newid(self):
		p = os.popen("uuidgen")
		self.nodeid = p.read()
		p.close()
		f = open("nodeid.dat", mode="w")
		f.write(self.nodeid)
		f.close()

	def addNode(self,node):
		self.nodesByAddr[node.addr] = node
		self.nodesById[node.nodeid] = node

metadata = NodeMetadata()

class MultiAnnouncer:
	def makesock(self):
		sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, 0)
		sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR,1)
		sock.bind(('', UDP_PORT))

		mreq = struct.pack("4sl", socket.inet_aton(MULTICAST_GROUP_ADDR), socket.INADDR_ANY)
		sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)
		self.sock = sock
	def run(self):
		while (1):
			for i in xrange(1,4):
				self.sendpacket()
				self.tickwait()
			self.expirenodes()

	def tickwait(self):
		tickstart = time.time()
		tickend = tickstart + TICK_TIME
		now = tickstart
		while (now < tickend):
			(readysocks,junk1,junk2) = select.select( [self.sock], [], [], (tickend - now))
			if (len(readysocks) > 0): 
				self.handlepacket()
			now = time.time()

	def sendpacket(self):
		data=":".join( 
			[MAGIC_BYTES,"hello",metadata.nodeid])
		flags=0
		self.sock.sendto(data, flags, (MULTICAST_GROUP_ADDR, UDP_PORT))

	def handlepacket(self):
		(data,addr) = self.sock.recvfrom(MAX_PACKET)
		# Check magic.
		if (data[0:len(MAGIC_BYTES)] != MAGIC_BYTES):
			return
		# Check colon.
		if (data[len(MAGIC_BYTES)] != ':'):
			return
		# split into fields
		(magic, command, cmddata) = data.split(":",3)
		# Decide what to do depending on command
		if (command == 'hello'):
			self.handlehello(addr, cmddata)
		# unknown cmd
	
	def handlehello(self,addr,cmddata):
		(ipaddr,port) = addr
		nodeid = cmddata
		#if (nodeid == metadata.nodeid) :
		#	# It's us.
		#	return
		# Find the node...
		if (nodeid in metadata.nodesById):
			# Already got it.
			# Update IP address and last seen time
			existingnode = metadata.nodesById[nodeid]
			if (existingnode.addr != ipaddr):
				print "Node %s changed IP address" % nodeid
				# Update IP addr
				# Remove old pointer
				del metadata.nodesByAddr[existingnode.addr]
				existingnode.addr = ipaddr
				# add new pointer
				metadata.nodesByAddr[ipaddr] = existingnode
			# Update last seen time
			existingnode.lastseen = time.time()
			existingnode.up = 1
		else:
			newnode = NodeInfo()
			newnode.nodeid = nodeid
			newnode.addr = ipaddr
			newnode.lastseen = time.time()
			metadata.addNode(newnode)
			print "New node: %s" % nodeid
	def expirenodes(self):
		now = time.time()
		totalnodes=0
		upnodes=0
		for id in metadata.nodesById:
			node = metadata.nodesById[id] 
			if (id == metadata.nodeid):
				# it's me
				pass
			else:
				age = now - node.lastseen 
				# check expiry
				if (age > MAX_NODE_AGE):
					# Mark node down.
					node.up=0
			totalnodes = totalnodes + 1
			if (node.up):
				upnodes = upnodes + 1
		print "Nodes up: %d / %d " % (upnodes, totalnodes)

def main():
	metadata.startup()
	m = MultiAnnouncer()
	m.makesock()
	m.run()

if __name__ == '__main__':
  main()



