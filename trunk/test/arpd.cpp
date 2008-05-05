#include "macaddress.h"
#include "ipaddress.h"

#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <sstream>
#include <cassert>

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

// This is probably a bit naughty.
#include <linux/if_ether.h>

static int sock;
static IpAddress our_arp_ip; // IP address we answer for
static MacAddress our_arp_mac; // MAC address we answer with
static MacAddress local_mac; // Local MAC of the interface
static int ifindex; // Index of the inferface (for bind and sending from)

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
	local_mac = MacAddress(ifr.ifr_hwaddr.sa_data);
	std::cout << "Local mac: " << local_mac << std::endl;
	// Determine ifindex of device
	memset(&ifr, sizeof(ifr), 0);
	std::strncpy(ifr.ifr_name, devname, IFNAMSIZ);
	res = ioctl(sock, SIOCGIFINDEX, &ifr);
	if (res != 0) {
		std::perror("ioctl SIOCGIFINDEX");
		exit(1);
	}
	ifindex = ifr.ifr_ifindex;
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

static void sendresponse(const MacAddress &sendermac, const IpAddress &senderip)
{
	// std::cout << "Sending response to " << senderip << std::endl;
	char buf[100];
	struct ethhdr * hdr = (struct ethhdr *) buf;
	memset(hdr, sizeof(struct ethhdr), 0);
	sendermac.copyTo((char *) (hdr->h_dest));
	// Spoof source of the mac that we're announcing.
	our_arp_mac.copyTo((char *) hdr->h_source);
	hdr->h_proto = htons(ETH_P_ARP);
	const size_t payloadlen = 
		8 // size of the arp header bits
		+ 6 + 4 // Sender (original sender, not us) mac+ip
		+ 6 + 4 // Target mac+ip
		;
	size_t totallen = sizeof(struct ethhdr) + payloadlen;
	assert(totallen < sizeof(buf));
	char *payload = (buf + sizeof(struct ethhdr));
	memset(payload, payloadlen, 0);
	*(unsigned short *) payload = htons(1); // hardware type 1 = ethernet
	*(unsigned short *) (payload + 2) = htons(ETH_P_IP); // protocol
	*(unsigned char *) (payload + 4) = 6; // hardware addr len = 6
	*(unsigned char *) (payload + 5) = 4; // protocol addr len = 4
	*(unsigned short *) (payload + 6) = htons(2); // Operation: 2 = reply
	our_arp_mac.copyTo(payload + 8);
	our_arp_ip.copyTo(payload + 8 + 6);
	sendermac.copyTo(payload + 8 + 6 + 4);
	senderip.copyTo(payload + 8 + 6 + 4 + 6);
	// Right. That's got the header + payload sorted out,
	// figure out how to send it. SOCK_PACKET addresses
	// are a sockaddr_ll which should be zero filled
	// and only the interface index is really used.
	struct sockaddr_ll sendaddr;
	memset(&sendaddr, sizeof(sendaddr), 0);
	sendaddr.sll_family = AF_PACKET;
	sendaddr.sll_halen = 6; // hardware addr len
	sendermac.copyTo((char *) sendaddr.sll_addr);
	sendaddr.sll_ifindex = ifindex;

	int res = sendto(sock, buf, totallen, 0, /* flags */
		(struct sockaddr *) &sendaddr, sizeof(sendaddr));
	if (res == -1) {
		std::perror("sendto");
		throw std::runtime_error("sendto fails");
	}
}

static void handlepacket(const char *buf, int len)
{
	// std::cout << "Got packet of length " << len << std::endl;
	// std::cout << "Dest:" << MacAddress(buf) << std::endl;
	// std::cout << "Src:" << MacAddress(buf+6) << std::endl;
	unsigned short proto = ntohs(*((unsigned short *) (buf+12)));
	if (proto != ETH_P_ARP) {
		return;
	}
	const char *arp = (buf + 14);
	unsigned short htype = ntohs(*(unsigned short *) arp);
	unsigned short ptype = ntohs(*(unsigned short *) (arp + 2));
	unsigned char hlen = *(unsigned char *) (arp+4);
	unsigned char plen = *(unsigned char *) (arp+5);
	unsigned short oper = ntohs(*((unsigned short *) (arp + 6)));
	// std::cout << "htype=" << htype << " ptype=" << ptype << 
	//	" hlen=" << (int) hlen << " plen=" << (int) plen << std::endl;
	// std::cout << "oper=" << oper << std::endl;
	if (htype != 1) {
		// Wrong htype.
		return;
	}
	if (ptype != ETH_P_IP) {
		// Wrong proto.
		return;
	}
	if (oper == 1) {
		// std::cout << "ARP rq" << std::endl;
		if ((hlen != 6) || (plen != 4)) {
			std::cout << "Unexpected lengths in arp, ignoring" << std::endl;
			return;
		}
		// ARP request.
		MacAddress sendermac(arp + 8);
		IpAddress senderip((struct in_addr *) (arp + 14));
		MacAddress targetmac(arp + 18);
		IpAddress targetip((struct in_addr *) (arp + 24));
		/*
		std::cout << "Sender MAC: " << sendermac << " IP: " << senderip
			<< std::endl;
		std::cout << "Target MAC: " << targetmac << " IP: " << targetip
			<< std::endl;
		*/
		// Determine whether the target IP is one we are interested in...
		if (targetip == our_arp_ip) {
			sendresponse(sendermac, senderip);
			return;
		}
		return;
	}
	if (oper == 2) {
		// ARP response
		return;
	}
	std::cout << "Unknown arp operation " << oper << std::endl;
	return;
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
		handlepacket(buf,len);
	}
}

int main(int argc, const char * argv[])
{
	if (argc < 4 ) {
		std::printf("args required: interface name, ip address, mac\n");
		return 1;
	}
	makesock(argv[1]);
	our_arp_ip.copyFromString(argv[2]);
	std::istringstream iss(argv[3]);
	iss >> our_arp_mac;
	getpackets();
}
