
#include "ipaddress.h"

class ClusterMembership {
	public:
		int sock; // used for multicast
		IpAddress clusteraddr;
	public:
		ClusterMembership(const IpAddress & bindaddr, const IpAddress & clusteraddr);
		void HandleMessage();
		void Tick();
};
