
#include <stdexcept>
#include <net/if.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <netinet/in.h>

#include "ifholder.h"
#include "utils.h"

InterfaceHolder::InterfaceHolder(int ifindex, const MacAddress & sharedmac)
{
     this->ifindex = ifindex;
     this->sharedmac = sharedmac;
     // We aren't actually interested in ARP packets
     // This socket is just to join a LL multicast group.
     // Or to set promiscuous mode.
     sock = socket(PF_PACKET,SOCK_RAW,htons(ETH_P_ARP));
     if (sock == -1) {
             throw std::runtime_error("raw socket");
     }
     struct packet_mreq pmr;
     memset(&pmr, sizeof(pmr),0);
     pmr.mr_ifindex = ifindex;
     // pmr.mr_type = PACKET_MR_MULTICAST;
     pmr.mr_type = PACKET_MR_PROMISC;
     pmr.mr_alen = 6;
     sharedmac.copyTo((char *) pmr.mr_address);
     int res = setsockopt(sock, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &pmr,
          sizeof(pmr));
     if (res != 0) {
          throw std::runtime_error("joining multicast / setting promisc ");
     }
	// So far so good. 
}

InterfaceHolder::~InterfaceHolder()
{
	close(sock);
}


/*
	int ifindex;
	MacAddress sharedmac;
	int sock;
*/
