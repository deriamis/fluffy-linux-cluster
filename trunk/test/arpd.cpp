
#include <cstdio>
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
	for (int i=0; i<6; i++) {
		std::printf("Byte %02x\n", (int)
			ifr.ifr_hwaddr.sa_data[i]);
	}
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
		std::printf("Got %d byte packet\n", len);

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
