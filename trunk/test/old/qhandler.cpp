#include <netinet/in.h>
#include <linux/netfilter.h>            /* for NF_ACCEPT */

extern "C" {
#include <libnetfilter_queue/libnetfilter_queue.h>
}

#include <cstring>
#include <iostream>
#include <stdexcept>
#include "ipaddress.h"
#include "fnv.h"

static const unsigned short QueueId = 42;

static int lowerHashLimit = 0;
static int upperHashLimit = 0x10000;

static u_int32_t get_pkt_id (struct nfq_data *tb)
{
        int id = 0;
        struct nfqnl_msg_packet_hdr *ph;

        ph = nfq_get_msg_packet_hdr(tb);
        if (ph){
                id = ntohl(ph->packet_id);
        }
        return id;
}

static int packet_callback(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg,
              struct nfq_data *nfa, void *data)
{
	u_int32_t id = get_pkt_id(nfa);
	char *payload;
	int payload_len;
	payload_len = nfq_get_payload(nfa, &payload);

	// Determine addresses
	IpAddress srcaddr((struct in_addr *) (payload + 12));
	IpAddress dstaddr((struct in_addr *) (payload + 16));
	std::cout << "Got packet ID " << id << 
		" from " << srcaddr << " to " << dstaddr << std::endl;
	// Work out what proto it is...
	int proto = (int) (unsigned char) payload[9];
	FnvHash hash;
	hash.addData(payload + 12, 8); // src + dst addr
	if (proto == IPPROTO_TCP) {
		unsigned short sport = ntohs( *(unsigned short *) (payload+20));
		unsigned short dport = ntohs( *(unsigned short *) (payload+22));
		std::cout << "TCP sport " << sport << " dport " << dport
			<< std::endl;
		hash.addData(payload + 20, 4); // src + dst ports
	}
	int hashvalue = hash.get16rev();
	std::cout << "Hash value=" << hashvalue << std::endl;
	u_int32_t verdict = NF_DROP;
	if ((hashvalue >= lowerHashLimit) && (hashvalue < upperHashLimit))
		verdict = NF_ACCEPT;
	return nfq_set_verdict(qh, id, verdict, 0, NULL);
}

static int run_daemon()
{
	struct nfq_handle *h;
	std::cout << "Opening library..." << std::endl;
	h = nfq_open();
	if (h == 0) { throw std::runtime_error("Cannot open lib"); }
	// Unbind existing handler
	if (nfq_unbind_pf(h, AF_INET) < 0) {
	    std::cerr << "error during nfq_unbind_pf()\n";
	}
	// Bind new handler
	if (nfq_bind_pf(h, AF_INET) < 0) {
	    throw std::runtime_error("nfq_bind_pf failed");
	}
	struct nfq_q_handle *qh = nfq_create_queue(h,  QueueId, &packet_callback, 0);
	if (qh == 0) { throw std::runtime_error("Cannot create queue"); }
	// Set packet copy mode...
	if (nfq_set_mode(qh, NFQNL_COPY_PACKET, 0xffff) < 0) {
	    throw std::runtime_error("nfq_set_mode failed");
	}

	// Get the nl handle and file descriptor...
	struct nfnl_handle *nh = nfq_nfnlh(h);
	int fd = nfnl_fd(nh);
	bool finished = false;
	std::cout << "entering main loop\n";
	while (! finished) {
		char buf[4096];
                int rv = recv(fd, buf, sizeof(buf), 0);
                if (rv > 0) {
                        nfq_handle_packet(h, buf, rv);
                } else {
                        perror("recv");
			throw std::runtime_error("recv failed");
                }
        }

	std::cout << "unbinding from queue\n";
        nfq_destroy_queue(qh);
	nfq_close(h);
	return 0;
}

int main(int argc, char *argv[])
{
	if (argc >= 3) {
		lowerHashLimit = std::atoi(argv[1]);
		upperHashLimit = std::atoi(argv[2]);
	}
	return run_daemon();
}
