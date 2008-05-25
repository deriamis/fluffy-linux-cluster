#include <cstdlib>
#include <iostream>
#include <stdexcept>

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <net/ethernet.h>
#include <netpacket/packet.h>
#include <poll.h>
#include <sys/time.h>

#include "fluffycluster.h"
#include "ifholder.h"
#include "membership.h"
#include "qhandler.h"

ClusterInfo clusterinfo;

static void initclustermac()
{
	// clusterinfo.macaddress = MacAddress::fromString("03:00:01:aa:aa:aa");
	clusterinfo.macaddress = MacAddress::fromString("02:00:01:aa:aa:aa");
	// Copy the last 3 bytes of the IP address into our mac.
	clusterinfo.macaddress.bytes[3] = 
		(char) ( (clusterinfo.ipaddress.ipnum >> 16) & 0xff);
	clusterinfo.macaddress.bytes[4] = 
		(char) ( (clusterinfo.ipaddress.ipnum >> 8) & 0xff);
	clusterinfo.macaddress.bytes[5] = 
		(char) ( clusterinfo.ipaddress.ipnum & 0xff);
	std::cout << "Cluster mac addr " << clusterinfo.macaddress << std::endl;
}

static void initinfo()
{
	// Make an unbound UDP socket so we can do some
	// IOCTLs on it.
	int sock = socket(PF_INET,SOCK_DGRAM,0);
	if (sock == -1) {
		throw std::runtime_error("socket");
	}
	// Get the interface index of our local dev
	struct ifreq ifr;
	memset(&ifr, sizeof(ifr),0);
	// Get the MAC address of our local dev.
	std::strncpy(ifr.ifr_name, clusterinfo.ifname, IFNAMSIZ);
	int res = ioctl(sock, SIOCGIFHWADDR, &ifr);
	if (res != 0) {
		std::perror("ioctl SIOCGIFHWADDR");
		throw std::runtime_error("Unable to get HW addr");
	}
	// Ensure that it's ethernet
	if (ifr.ifr_hwaddr.sa_family != ARPHRD_ETHER) {
		throw std::runtime_error("Interface does not appear to be ethernet");
	}
	// Save the mac addr.
	clusterinfo.localmac = MacAddress(ifr.ifr_hwaddr.sa_data);
	// Get the i/f index
	memset(&ifr, sizeof(ifr),0);
	std::strncpy(ifr.ifr_name, clusterinfo.ifname, IFNAMSIZ);
	res = ioctl(sock, SIOCGIFINDEX, &ifr);
	if (res != 0) {
		std::perror("ioctl SIOCGIFINDEX");
		throw std::runtime_error("Unable to get IF index");
	}
	clusterinfo.ifindex =  ifr.ifr_ifindex;
	// Get the primary IP address of the device
	memset(&ifr, sizeof(ifr),0);
	res = ioctl(sock, SIOCGIFADDR, &ifr);
	if (res != 0) {
		throw std::runtime_error("SIOCGIFADDR");
	}
	struct sockaddr_in * plocaladdr = (struct sockaddr_in *) &ifr.ifr_addr;

	clusterinfo.localip.copyFromInAddr(& (plocaladdr->sin_addr));
	close(sock);
	initclustermac();

	std::cout << "Interface: " << clusterinfo.ifname << std::endl
		<< "Index: " << clusterinfo.ifindex << std::endl
		<< "Local MAC: " << clusterinfo.localmac << std::endl
		<< "Local IP: " << clusterinfo.localip << std::endl;
}

static void mainloop(int initialWeight)
{
	ClusterMembership membership(clusterinfo.localip, clusterinfo.ipaddress,
		clusterinfo.ifindex);
	membership.weight = initialWeight;
	QHandler qhand(42); // Queue ID
	membership.qhand = &qhand;
	std::cout << "I enter the main loop here \n";
	bool finished = false;
	struct timeval nexttick;
	gettimeofday(&nexttick, 0);
	while (! finished) {
		struct pollfd fds[2];
		fds[0].fd = qhand.sock;
		fds[0].events = POLLIN | POLLERR;
		fds[0].revents = 0;
		fds[1].fd = membership.sock;
		fds[1].events = POLLIN | POLLERR;
		fds[1].revents = 0;
		int timeleft = 1000; // milliseconds
		struct timeval now;
		gettimeofday(&now, 0);
		timeleft = (nexttick.tv_sec - now.tv_sec) * 1000 +
			((nexttick.tv_usec - now.tv_usec) / 1000);
		if (timeleft <= 0) {
			membership.Tick();
			nexttick.tv_sec = now.tv_sec + 1;
			nexttick.tv_usec = now.tv_usec;
		} else {
			int res = poll(fds, 2, timeleft);
			if (res == -1) {
				throw std::runtime_error("poll failed");
			}
			if (fds[0].revents != 0) {
				qhand.HandleMessage();
			}
			if (fds[1].revents != 0) {
				membership.HandleMessage();
			}
		}
	}
}

static void runcluster(int initialWeight)
{
	initinfo();
	InterfaceHolder ifholder(clusterinfo.ifindex, clusterinfo.macaddress);
	mainloop(initialWeight);
}

int main(int argc, const char *argv[])
{
	if (argc < 3) {
		throw std::runtime_error("Need 2 args, interface name and ip addr");
	}
	clusterinfo.ifname = argv[1];
	clusterinfo.ipaddress.copyFromString(argv[2]);
	int initialWeight = 100;
	if (argc > 3) {
		initialWeight = std::atoi(argv[3]);
	}
	runcluster(initialWeight);
}
