
#include "ipaddress.h"
#include <map>

class QHandler;

struct NodeInfo {
	bool isme;
	bool isup;
	bool dead; // pending removal from the map.
	int weight;
	struct timeval lastheard; // last heard time
	int lowerHashLimit, upperHashLimit; // hash boundaries

	NodeInfo() : isme(false), isup(false), dead(false), weight(0) { 
		lastheard.tv_sec = 0;	
		lastheard.tv_usec = 0;	
	}
};

typedef std::map <IpAddress, NodeInfo> IpNodeMap;

class ClusterMembership {
	public:
		int sock; // used for multicast
		IpAddress clusteraddr;
		IpAddress localaddr;
		IpAddress master;
		int weight;
		int effectiveWeight;
		struct timeval starttime;
		QHandler * qhand;
	public:
		ClusterMembership(const IpAddress & bindaddr, const IpAddress & clusteraddr, int ifindex);
		void HandleMessage();
		void Tick();
	private:
		int buildPacket(char *buf, int maxlen);
		void decodePacket(const char *buf, int len, const IpAddress &src);
		void decodeMasterPacket(const char *buf, int len, const IpAddress &src);
		void calcBoundaries();
		void setNewLocalBoundaries(int lower, int upper);
		IpNodeMap nodes;
};


