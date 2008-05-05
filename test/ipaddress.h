#include <set>
#include <string>
#include <arpa/inet.h>

struct IpAddress {
	unsigned long ipnum; // In HOST order not net order; convert when needed
	std::string toString() const;

	void copyTo(char * to) const {
		* (unsigned long *)to = htonl(ipnum);
	};
	void copyToInAddr(struct in_addr * to) const;
	void copyFromInAddr(const struct in_addr * from);
	void copyFromString(const char *str);
	void chopNetmask(int bits); // Chops all the bits beyond the first bits off
	void operator ++ ();

	bool operator < (const IpAddress & other) const {
		return ipnum < other.ipnum;
	}
	bool operator > (const IpAddress & other) const {
		return ipnum > other.ipnum;
	}
	IpAddress(const struct in_addr *from) {
		copyFromInAddr(from);
	};
	IpAddress() : ipnum(0) {
	};
	bool operator == (const IpAddress &other) {
		return ipnum == other.ipnum;
	};
};

typedef std::set<IpAddress> IpAddressSet;
typedef std::set<IpAddress>::const_iterator IpAddressSetIterator;

std::ostream & operator << (std::ostream &out, const IpAddress &addr);

