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

#include "fluffycluster.h"

ClusterInfo clusterinfo;

static void initmulticastll()
{
	clusterinfo.macaddress = MacAddress::fromString("03:00:01:aa:aa:aa");
	// Copy the last 3 bytes of the IP address into our mac.
	clusterinfo.macaddress.bytes[3] = 
		(char) ( (clusterinfo.ipaddress.ipnum >> 16) & 0xff);
	clusterinfo.macaddress.bytes[4] = 
		(char) ( (clusterinfo.ipaddress.ipnum >> 8) & 0xff);
	clusterinfo.macaddress.bytes[5] = 
		(char) ( clusterinfo.ipaddress.ipnum & 0xff);
	std::cout << "Our multicast addr " << clusterinfo.macaddress << std::endl;
	// We aren't actually interested in ARP packets
	// This socket is just to join a LL multicast group.
	int sock = socket(PF_PACKET,SOCK_RAW,htons(ETH_P_ARP));
	if (sock == -1) {
		throw std::runtime_error("raw socket");
	}
	struct packet_mreq pmr;
	memset(&pmr, sizeof(pmr),0);
	pmr.mr_ifindex = clusterinfo.ifindex;
	pmr.mr_type = PACKET_MR_MULTICAST;
	pmr.mr_alen = 6;
	clusterinfo.macaddress.copyTo((char *) pmr.mr_address);
	int res = setsockopt(sock, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &pmr,
		sizeof(pmr));
	if (res != 0) {
		throw std::runtime_error("joining ll multi group");
	}
	std::cout << "Great joined ll multi group \n";
	sleep(600);
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
	initmulticastll();
	std::cout << "Interface: " << clusterinfo.ifname << std::endl
		<< "Index: " << clusterinfo.ifindex << std::endl
		<< "Local MAC: " << clusterinfo.localmac << std::endl
		<< "Local IP: " << clusterinfo.localip << std::endl;
}

int main(int argc, const char *argv[])
{
	if (argc < 3) {
		throw std::runtime_error("Need 2 args, interface name and ip addr");
	}
	clusterinfo.ifname = argv[1];
	clusterinfo.ipaddress.copyFromString(argv[2]);
	initinfo();
}
