#ifndef MACADDRESS_INCLUDED
#define MACADDRESS_INCLUDED 

#include <cstring>
#include <ostream>

struct MacAddress {
	char bytes[6];

	MacAddress(const char *ptr) {
		std::memcpy(bytes, ptr, 6);
	}
	MacAddress() {
		std::memset(bytes, 0, 6);
	}

	bool operator < (const MacAddress &other)  const
	{
		int c = std::memcmp(bytes,other.bytes,6);
		return (c < 0);
	}
	bool operator == (const MacAddress &other)  const
	{
		int c = std::memcmp(bytes,other.bytes,6);
		return (c == 0);
	}
	
	void copyTo(char *ptr) const
	{
		std::memcpy(ptr, bytes, 6);
	}
	static MacAddress fromString(const char *str);	
};
// Declaration
std::ostream & operator << (std::ostream &out, const MacAddress &addr);

std::istream & operator >> (std::istream &in, MacAddress &addr);

#endif // MACADDRESS_INCLUDED 

