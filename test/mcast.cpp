
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/socket.h>
#include <netinet/ip.h> 
#include <arpa/inet.h> 
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>
#include <poll.h>
#include <iostream>

#include "ipaddress.h"

static const unsigned int ourport = 15987;
static in_addr multiaddr;

static int sock;

static void makesock()
{
	sock = socket(PF_INET,SOCK_DGRAM,0);
	if (sock == -1) {
		std::perror("socket");
		exit(1);
	}
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(ourport);
	addr.sin_addr.s_addr = INADDR_ANY;
	if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
		std::perror("bind");
		std::exit(1);
	}

}

void multisetup()
{
	inet_aton("239.255.42.99", &multiaddr);
	struct ip_mreqn mr;
	mr.imr_multiaddr = multiaddr;
	mr.imr_address.s_addr = INADDR_ANY;
	mr.imr_ifindex = 0;
	int res = setsockopt(sock, IPPROTO_IP, 
			IP_ADD_MEMBERSHIP,
			& mr,
			sizeof(mr));
	if (res == -1) {
		std::perror("join");
		std::exit(2);
	}
	std::cout << "So far so good\n";
}

static void dotick()
{
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
	}
	std::cout << "Tick\n";
}

static void readpacket()
{
	struct sockaddr_in src;
	char buf[300];
	size_t srclen = sizeof(src);
	int len = recvfrom(sock, buf, sizeof(buf), 0 /* flags*/ ,
			(struct sockaddr *) &src, &srclen);
	if (len == -1) {
		std::perror("recvfrom");
	} else {

		std::cout << "Got datagram of " << len << " bytes from "
			<< IpAddress(& (src.sin_addr)) << std::endl;
	}
}

static void srloop()
{
	sigset_t maskedsigs;
	sigemptyset(&maskedsigs);
	struct timeval nexttick;
	gettimeofday(&nexttick, 0);
	while (1) {
		struct pollfd fds[1];
		fds[0].fd = sock;
		fds[0].events = POLLIN | POLLERR;
		fds[0].revents = 0;
		
		struct timeval now;
		gettimeofday(&now, 0);
		int timeleft; /* ms */
		timeleft = (nexttick.tv_sec - now.tv_sec) * 1000 +
			((nexttick.tv_usec - now.tv_usec) / 1000);
		if (timeleft <= 0) {
			dotick();
			nexttick.tv_sec = now.tv_sec + 1;
			nexttick.tv_usec = now.tv_usec;
		} else {
			int res = poll(fds, 1, timeleft);
		}
		// int res = ppoll(fds, 1, &timeout, &maskedsigs);
		if (fds[0].revents & (POLLIN | POLLERR)) {
			readpacket();
		}
	}
}


int main(int argc, char * argv)
{
	makesock();
	multisetup();
	srloop();

}
