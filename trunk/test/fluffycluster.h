
#include "ipaddress.h"
#include "macaddress.h"

// Just one of these (clusterinfo) provides
// A set of useful stuff about the entire cluster
// and the current node.
struct ClusterInfo {
	// IP address of the cluster
	IpAddress ipaddress;
	// MAC address we invented for the cluster
	MacAddress macaddress;
	// Local interface we're using
	const char *ifname;
	int ifindex;
	// Local IP address for our own node
	IpAddress localip;
	MacAddress localmac;
};

// These are populated into a map from IP addresses of
// nodes.
struct NodeInfo {
	IpAddress ipaddress;
	bool up; // Whether node is currently "up"
	int weight; // Load balanced weight
	

};

extern ClusterInfo clusterinfo;

