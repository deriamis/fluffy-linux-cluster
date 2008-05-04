#include "macaddress.h"

#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <cstring>
#include <sys/socket.h>
#include <netinet/ip.h> 
#include <netpacket/packet.h> 
#include <net/ethernet.h> 
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <net/if.h>

static int sock;

void makesock(const char *devname)
{
	sock = socket(PF_PACKET,SOCK_RAW,htons(ETH_P_ARP));
	if (sock == -1) {
		std::perror("socket");
		exit(1);
	}
	// TODO: bind
	// Determine MAC address of dev
	struct ifreq ifr;
	memset(&ifr, sizeof(ifr), 0);
	std::strncpy(ifr.ifr_name, devname, IFNAMSIZ);
	int res = ioctl(sock, SIOCGIFHWADDR, &ifr);
	if (res != 0) {
		std::perror("ioctl SIOCGIFHWADDR");
		exit(1);
	}
	std::printf("Family: %08x\n", ifr.ifr_hwaddr.sa_family);
	MacAddress ourmac(ifr.ifr_hwaddr.sa_data);
	std::cout << "Our mac: " << ourmac << std::endl;
	// Determine ifindex of device
	memset(&ifr, sizeof(ifr), 0);
	std::strncpy(ifr.ifr_name, devname, IFNAMSIZ);
	res = ioctl(sock, SIOCGIFINDEX, &ifr);
	if (res != 0) {
		std::perror("ioctl SIOCGIFINDEX");
		exit(1);
	}
	int ifindex = ifr.ifr_ifindex;
	std::cout << "Index " << ifindex << std::endl;
	struct sockaddr_ll bindaddr;
	memset(&bindaddr, sizeof(bindaddr),0);
	bindaddr.sll_family = AF_PACKET;
	bindaddr.sll_protocol = htons(ETH_P_ARP);
	bindaddr.sll_ifindex = ifindex;
	res = bind(sock, (struct sockaddr *) &bindaddr, sizeof(bindaddr));
	if (res != 0) {
		std::perror("bind");
		exit(1);
	}
	
	
}

static void printhex(const char *buf, int len)
{
		std::ios_base::fmtflags oldflags;
		oldflags = std::cout.flags(std::ios_base::hex);
		std::cout << std::setfill('0');

		for (int n = 0; n < len; n++) {
			if ((n > 0) && ((n % 8) == 0)) {
				std::cout << std::endl;
			}
			std::cout << std::setw(2) << ((int) (unsigned char) buf[n]);
		}
		std::cout << std::endl;
		std::cout.flags(oldflags);

}
static void getpackets()
{
	char buf[100];
	while (1) {
		sockaddr_ll sender;
		socklen_t senderlen = sizeof(sender);
		int len = recvfrom(sock, buf, sizeof(buf), 0 /* flags */,
				(struct sockaddr *) &sender, &senderlen);
		if (len == -1) {
			perror("recvfrom");
			exit(2);
		}
		std::cout << "Got packet of length " << len << std::endl;
		printhex(buf,len);
	}
}

int main(int argc, const char * argv[])
{
	if (argc < 2 ) {
		std::printf("arg required.\n");
		return 1;
	}
	makesock(argv[1]);
	getpackets();
}
