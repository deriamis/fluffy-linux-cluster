#include <sstream>
#include <iostream>
#include <stdexcept>

#include "ipaddress.h"

using namespace std;

string IpAddress::toString() const {
	ostringstream oss;
	oss << (*this);
	return oss.str();
}

void IpAddress::copyToInAddr(struct in_addr * to) const
{
	u_int32_t netorder = htonl(ipnum);
	to->s_addr = netorder;
}

void IpAddress::copyFromInAddr(const struct in_addr * from)
{
	u_int32_t hostorder = ntohl(from->s_addr);
	ipnum = hostorder;
}

void IpAddress::copyFromString(const char *str)
{
	struct in_addr addr;
	if (inet_aton(str, &addr) == 0) {
		throw std::runtime_error("Invalid IP address copyFromString");
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

std::ostream & operator << (std::ostream &out, const IpAddress &addr)
{
	unsigned char bytes[4];
	addr.copyTo((char *) bytes);
        for (int i=0; i<4; i++) {
		if (i > 0) {
			out << '.';
		}
                out << (int) (bytes[i]);
        }
        return out;
}
