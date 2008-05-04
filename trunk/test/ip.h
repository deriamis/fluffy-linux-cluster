#include <set>
#include <string>
#include <arpa/inet.h>

struct IpAddress {
	unsigned long ipnum;
	std::string toString() const;

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
	IpAddress(const char *str) {
		copyFromString(str);
	};
};

typedef std::set<IpAddress> IpAddressSet;
typedef std::set<IpAddress>::const_iterator IpAddressSetIterator;

