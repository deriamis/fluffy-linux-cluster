
#include "ipaddress.h"

// Some forward decls so callers don't have to pull in
// all the netfilter queue stuff.
struct nfq_handle;
struct nfnl_handle;
struct nfq_q_handle;

class QHandler {
	public:
		int sock; // netlink socket
		int lowerHashLimit;
		int upperHashLimit;
	public:
		QHandler(unsigned short qid);
		void HandleMessage();
		void Tick();
		~QHandler();
		struct nfq_q_handle *GetQh() { return qh; }
	private:
		struct nfq_handle *h;
		struct nfnl_handle *nh;
		struct nfq_q_handle *qh;
		unsigned short qid;
};
