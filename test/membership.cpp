#include "membership.h"

#include "ipaddress.h"
#include "utils.h"
#include "timing.h"
#include "qhandler.h"

#include <sys/time.h>

#include <stdexcept>
#include <errno.h>
#include <iostream>

static const unsigned int ourport = 15987;
static in_addr multiaddr;

enum CommandType { CommandStatus =1, CommandMaster = 2 };
static const unsigned int MagicNumber = 0x09bea717;

static const int MaxNodeAge = 3500; // milliseconds
static const int ForgetNodeAge = 600000; // milliseconds
static const int ZeroWeightTime = 4500; // milliseconds
// The maximum number of nodes controls the max packet size etc.
// This is set arbitrarily, the size needed is approximately
// MaxNodes * 12  + 21 (i.e. 1221 current), which of course
// needs to stay under the maximum UDP datagram (63kish)
// However, it is also handy if it stays under the ethernet MTU
// with headers etc, which is 1500 bytes, so 100 is about right.
// Also, with 100 nodes, a lot of traffic will be going to all
// the nodes, possibly causing scalability issues (depending on 
// the application)
static const int MaxNodes = 100;

static int makesock(const IpAddress & bindaddr)
{
        int sock = socket(PF_INET,SOCK_DGRAM,0);
        if (sock == -1) {
                throw std::runtime_error("dgram socket");
        }
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(ourport);
	// bindaddr.copyToInAddr(& addr.sin_addr);
	addr.sin_addr.s_addr = INADDR_ANY;
        if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
                throw std::runtime_error("bind");
        }
	return sock;
}
static void multisetup(int sock, const IpAddress &bindaddr, int ifindex)
{
        inet_aton("239.255.42.99", &multiaddr);
        struct ip_mreqn mr;
        mr.imr_multiaddr = multiaddr;
	bindaddr.copyToInAddr(& mr.imr_address);
        mr.imr_ifindex = ifindex;
        int res = setsockopt(sock, IPPROTO_IP,
                        IP_ADD_MEMBERSHIP,
                        & mr,
                        sizeof(mr));
        if (res == -1) {
                throw std::runtime_error("join multicast grp");
        }
	int loop = 1;
	res = setsockopt(sock, IPPROTO_IP,
		IP_MULTICAST_LOOP, &loop, sizeof(loop));
        if (res == -1) {
                throw std::runtime_error("multicast loop");
        }
	res = setsockopt(sock, IPPROTO_IP,
		IP_MULTICAST_IF, &mr, sizeof(mr));
        if (res == -1) {
                throw std::runtime_error("multicast interface");
        }
}

ClusterMembership::ClusterMembership(const IpAddress &bindaddr, const IpAddress &clusteraddr, int ifindex)
{
	// Set qhand to null initially
	qhand = 0;
	// Open multicast socket...
	sock = makesock(bindaddr);
	// Join multicast group(s)
	multisetup(sock, bindaddr, ifindex);
	// Make it nonblocking.
	SetNonBlocking(sock);
	this->clusteraddr = clusteraddr;
	this->localaddr = bindaddr;
	// Record the startup time
	gettimeofday(& starttime, 0);
	effectiveWeight = 0;
	// Add our own host to "nodes"
	nodes[bindaddr] = NodeInfo();
	nodes[bindaddr].isme = true;
	nodes[bindaddr].isup = true;
	nodes[bindaddr].weight = effectiveWeight;
}

void ClusterMembership::HandleMessage()
{
	char buf[1024];
	// Read message from sock
	struct sockaddr_in sender;
	socklen_t senderlen = sizeof(sender);
	int len = recvfrom(sock, buf, sizeof(buf),
			0 /* flags */ ,
			(struct sockaddr *) &sender, &senderlen);
	// If we get EAGAIN, ignore
	if (len == -1) {
		if (errno == EAGAIN) {
			return;
		}
		throw std::runtime_error("recv failed");
	}
	// decide what to do with it etc
	decodePacket(buf,len, IpAddress(& sender.sin_addr));
}

void ClusterMembership::Tick()
{
	// Called every 1s or so to time out
	// to handle expired nodes etc
        struct sockaddr_in dst;
        dst.sin_family = AF_INET;
        dst.sin_port = htons(ourport);
        dst.sin_addr = multiaddr;
        const int buflen = 1400;
        char msg[buflen];
        std::memset(msg,0,buflen);
	int msglen = buildPacket(msg, buflen);

        int res = sendto(sock, msg, msglen, 0 /* flags */,
                (struct sockaddr *) & dst, sizeof(dst));
        if (res != msglen) {
                std::perror("sendto");
		throw std::runtime_error("sendto");
        }
	struct timeval now;
	gettimeofday(&now, 0);
	// Set the local node effective weight.
	int timesincestart = timevalSub(now, starttime);
	if (timesincestart < ZeroWeightTime) {
		effectiveWeight = 0;
	} else {
		effectiveWeight = weight;
	}
	// Store the effective weight so we can take it into account
	// determining if we are to be master or not
	nodes[localaddr].weight = effectiveWeight;

	// Calculate IP of the new master, to see if it's
	// us.
	IpAddress newMaster;
	for (IpNodeMap::iterator i=nodes.begin(); i != nodes.end(); i++) {
		const IpAddress & ip = (*i).first;
		NodeInfo &ni = (*i).second;
		int age = timevalSub(now, ni.lastheard);
		if (ni.isup && ! ni.isme) {
			if (age > MaxNodeAge) {
				std::cout << "Node down:" << ip << std::endl;
				ni.isup = false;
			}
		}
		if (ni.isup && (ni.weight > 0)) {
			if (newMaster == IpAddress()) {
				newMaster = ip;
			}
		}
	}
       	if (newMaster != master) {
		master = newMaster;
		std::cout << "New master: " << master << std::endl;
	}
}

int ClusterMembership::buildPacket(char *buf, int maxlen)
{
	int l = 0;
	// Magic number
	*(unsigned int *) buf = htonl(MagicNumber);
	l += 4;
	// Cluster IP
	clusteraddr.copyTo(& (buf[l])); l += 4;
	// Our IP
	localaddr.copyTo(& (buf[l])); l += 4;
	bool ismaster = (localaddr == master);
	CommandType cmd = CommandStatus;
	if (ismaster) {
		cmd = CommandMaster;
	}
	// Command type	
	*(unsigned int *) & buf[l] = htonl(cmd); l += 4;
	// Node weight
	*(int *) & buf[l] = htonl(effectiveWeight); l += 4;
	// If we're the master, add the number of nodes 
	// followed by their limits...
	if (ismaster) {
		calcBoundaries();
		int nodenumOffset = l; // Reserve a byte for num nodes
		l = l + 1;
		int numnodes = 0;
		for (IpNodeMap::iterator i=nodes.begin(); i != nodes.end(); i++) {
			if (l > (maxlen - 16)) {
				throw std::runtime_error("Buffer is in danger of overflow");
			}
			const IpAddress & ip = (*i).first;
			NodeInfo &ni = (*i).second;
			if (ni.isup && (ni.upperHashLimit != 0)) {
				ip.copyTo(& (buf[l])); l += 4;
				*(int *) & buf[l] = htonl(ni.lowerHashLimit); l += 4;
				*(int *) & buf[l] = htonl(ni.upperHashLimit); l += 4;
				numnodes ++;
			}
		}
		// Add number of nodes.
		buf[nodenumOffset] = (char) numnodes;
	}
	return l;
}

// As the master, calculate new hash boundaries...
void ClusterMembership::calcBoundaries()
{
	int totalweight = 0;
	// Store our own weight in case it changed...
	nodes[localaddr].weight = weight;
	for (IpNodeMap::iterator i=nodes.begin(); i != nodes.end(); i++) {
		NodeInfo &ni = (*i).second;
		if (ni.isup) {
			totalweight += ni.weight;
		}
	}
	const int BoundarySize = 0x10000;
	int n = 0;
	int num = 0;
	for (IpNodeMap::iterator i=nodes.begin(); i != nodes.end(); i++) {
		// const IpAddress & ip = (*i).first;
		NodeInfo &ni = (*i).second;
		if (ni.isup && (ni.weight > 0) && (num < MaxNodes)) {
			ni.lowerHashLimit = n / totalweight;
			n += ( BoundarySize * ni.weight);
			ni.upperHashLimit = n / totalweight;
			/* std::cout << "Node:" << ip << " Boundaries: " << 
				ni.lowerHashLimit << " to " << ni.upperHashLimit <<
				std::endl;
			*/
			num ++;
		} else {
			ni.lowerHashLimit = 0;
			ni.upperHashLimit = 0;
		}
	}
	const NodeInfo &localnode = nodes[localaddr];
	setNewLocalBoundaries(localnode.lowerHashLimit, 
		localnode.upperHashLimit);
	// std::cout << "Master active nodes: " << num << std::endl;
}

void ClusterMembership::decodePacket(const char *buf, int len, const IpAddress & src)
{
	// Check length acceptable
	if (len < 20) return;
	// Check magic number
	unsigned int magic = ntohl(* (unsigned int *) buf);
	if (magic != MagicNumber) return;
	// Check cluster IP
	IpAddress decode_clusteraddr;
	decode_clusteraddr.copyFromInAddr((struct in_addr *) (buf+4));
	if (decode_clusteraddr != clusteraddr) return;
	// Get source addr in packet, ensure it's the same as
	// the sender.
	IpAddress decode_src;
	decode_src.copyFromInAddr((struct in_addr *) (buf+8));
	if (decode_src != src) return;
	CommandType cmd = (CommandType) ntohl(* (unsigned int *) (buf + 12));
	unsigned int weight = ntohl(*(int *) (buf + 16));
	// Ignore messages from my own node.
	if (src == localaddr) return;
	switch (cmd) {
		case CommandStatus:
		case CommandMaster:
			// ok;
			break;
		default:
			// unknown cmd
			return;
	}
	// std::cout << "From " << src << " Command:" << cmd << " node weight:" << weight << std::endl;
	// Check if it's already in nodemap.
	if (nodes.find(src) == nodes.end())
	{
		nodes[src] = NodeInfo();
	}
	// Update weight and timestamp.
	nodes[src].weight = weight;
	if (! nodes[src].isup) {
		nodes[src].isup = true;
		std::cout << "Node up:" << src << std::endl;
	}
	// Timestamp 
	gettimeofday(& (nodes[src].lastheard), 0);
	if (cmd == CommandMaster) {
		decodeMasterPacket(buf,len,src);
	}
}

void ClusterMembership::decodeMasterPacket(const char *buf, int len, 
	const IpAddress & src)
{
	if (len < 21) {
		// too small.
		return;
	}
	int numnodes = (int) (buf[20]);
	// A master announcement is only helpful if there are at least
	// two nodes in it, and fewer than the max
	if ((numnodes < 2) || (numnodes > MaxNodes)) {
		return;
	}
	// We need 12 bytes per node
	if (len < (21 + (12 * numnodes))) {
		// still too small.
		return;
	}
	int newLower = 0;
	int newUpper = 0;
	const char *nodebuf = (buf + 21);
	for (int i=0; i< numnodes; i++) {
		int offset = (i * 12);
		IpAddress ip;
		ip.copyFromInAddr((struct in_addr *) (nodebuf+offset));
		if (ip == localaddr) {
			newLower = ntohl(*(int *)(nodebuf + offset + 4));
			newUpper = ntohl(*(int *)(nodebuf + offset + 8));
		}
	}
	setNewLocalBoundaries(newLower, newUpper);
}

void ClusterMembership::setNewLocalBoundaries(int lower, int upper)
{
	std::cout << "New boundaries: " << lower << " to " << upper << std::endl;
	if (qhand != 0) {
		qhand->lowerHashLimit = lower;
		qhand->upperHashLimit = upper;
	}
}


