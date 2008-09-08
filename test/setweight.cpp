#include "ipaddress.h"
#include "utils.h"

#include <sys/time.h>

#include <stdexcept>
#include <errno.h>
#include <iostream>
#include <cstdlib>

static const unsigned int ourport = 15987;

enum CommandType { CommandStatus =1, CommandMaster = 2, 
	CommandSetWeight = 3, CommandSetWeightResponse = 4 };
static const unsigned int MagicNumber = 0x09bea717;

static int makesock()
{
        int sock = socket(PF_INET,SOCK_DGRAM,0);
        if (sock == -1) {
                throw std::runtime_error("dgram socket");
        }
	return sock;
}

static void sendWeightPacket(int sock, int desiredWeight)
{
	char buf[200];
	int l = 0;
	memset(buf, 0, sizeof(buf));
	// Magic number
	*(unsigned int *) buf = htonl(MagicNumber);
	l += 4;
	// Cluster IP (4 bytes)
	l += 4;
	// Our IP (4 bytes);
	l += 4;
	CommandType cmd = CommandSetWeight;
	// Command type	
	*(unsigned int *) & buf[l] = htonl(cmd); l += 4;
	// Node weight
	*(int *) & buf[l] = htonl(desiredWeight); l += 4;
	// l gives us the desired length of packet.
	struct sockaddr_in dest;
	dest.sin_family = AF_INET;
	dest.sin_port = htons(ourport);
	IpAddress localhost;
	localhost.copyFromString("127.0.0.1");
	localhost.copyToInAddr(& dest.sin_addr);
	int r = sendto(sock, buf, l, 0 /* flags */ , (struct sockaddr *) &dest, sizeof(dest) );
	if (r < 0) {
		int senderr = errno;
		std::cerr << "Send failed: " << strerror(senderr) << std::endl; 
	}
	return;
}

int main(int argc, const char *argv[])
{
	int desiredWeight = 100;
	if (argc < 2) {
		throw std::runtime_error("Need at least 1 arg - specify desired weight");
	}
	desiredWeight = std::atoi(argv[1]);
	int sock = makesock();
	sendWeightPacket(sock, desiredWeight);
	return 0;
}

