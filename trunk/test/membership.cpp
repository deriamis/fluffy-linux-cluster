#include "membership.h"

#include "ipaddress.h"
#include "utils.h"

#include <stdexcept>
#include <errno.h>
#include <iostream>

static const unsigned int ourport = 15987;
static in_addr multiaddr;

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
static void multisetup(int sock, const IpAddress &bindaddr)
{
        inet_aton("239.255.42.99", &multiaddr);
        struct ip_mreqn mr;
        mr.imr_multiaddr = multiaddr;
	bindaddr.copyToInAddr(& mr.imr_address);
        mr.imr_ifindex = 0;
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
}

ClusterMembership::ClusterMembership(const IpAddress &bindaddr, const IpAddress &clusteraddr)
{
	// Open multicast socket...
	sock = makesock(bindaddr);
	// Join multicast group(s)
	multisetup(sock, bindaddr);
	// Make it nonblocking.
	SetNonBlocking(sock);
	this->clusteraddr = clusteraddr;
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
	std::cout << "Got packet length:" << len << std::endl;
}

void ClusterMembership::Tick()
{
	// Called every 1s or so to time out
	// to handle expired nodes etc
        struct sockaddr_in dst;
        dst.sin_family = AF_INET;
        dst.sin_port = htons(ourport);
        dst.sin_addr = multiaddr;
        const int msglen = 10;
        char msg[msglen];
        std::memset(msg,0,msglen);

        int res = sendto(sock, msg, msglen, 0 /* flags */,
                (struct sockaddr *) & dst, sizeof(dst));
        if (res != msglen) {
                std::perror("sendto");
		throw std::runtime_error("sendto");
        }
        std::cout << "Tick\n";
}

