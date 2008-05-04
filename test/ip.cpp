#include <sstream>
#include <iostream>

#include "ip.h"
#include "misc.h"

using namespace std;

string IpAddress::toString() const {
	ostringstream oss;
	for (int i=3; i>=0; i--) {
		if (i<3) oss << ".";
		unsigned long octet = (ipnum >> (i* 8)) & 0xff;
		oss << octet;
	}
	return oss.str();
}

// less than operator for the types.
bool LtIpAddress::operator()(const IpAddress a1, const IpAddress a2) const
{
	return a1.ipnum < a2.ipnum;
}

void IpAddress::copyToInAddr(struct in_addr * to) const
{
	unsigned char *toOctets = (unsigned char *) &(to->s_addr);
	for (int i=0; i<4; i++) {
		toOctets[i] = (ipnum >> ((3-i)*8)) & 0xff;
	}
}

void IpAddress::copyFromInAddr(const struct in_addr * from)
{
	unsigned char *fromOctets = (unsigned char *) &(from->s_addr);
	ipnum = 0;
	for (int i=0; i<4; i++) {
		ipnum = ipnum << 8;
		ipnum |= fromOctets[i];
	}
}

void IpAddress::copyFromString(const char *str)
{
	struct in_addr addr;
	if (inet_aton(str, &addr) == 0) {
		throw Exception("Invalid IP address copyFromString");
	}
	copyFromInAddr(&addr);
}

void IpAddress::chopNetmask(int bits)
{
	unsigned long netmask = 0xffffffff;
	netmask = netmask << (32 - bits);
	ipnum = ipnum & netmask;
}

void IpAddress::operator ++ ()
{
	ipnum++;
}
