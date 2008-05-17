#include "macaddress.h"
#include <iomanip>
#include <sstream>

std::ostream & operator << (std::ostream &out, const MacAddress &addr)
{
	std::ios_base::fmtflags oldflags;
	oldflags = out.flags(std::ios_base::hex);
	out << std::setfill('0');
	for (int i=0; i<6; i++) {
		out << std::setw(2) << (int) ((unsigned char) addr.bytes[i]);
		if (i < 5) {
			out << ':';
		}
	}
	out.flags(oldflags);
	return out;
}

std::istream & operator >> (std::istream &in, MacAddress &addr)
{
	std::string str;
	in >> str;
	for (int i=0; i<6; i++) {
		std::string chunk(str,i*3, 2);
		std::istringstream tempstream(chunk);
		tempstream >> std::hex;
		// rewind.
		unsigned int n(42);
		tempstream >> n; 
		addr.bytes[i] = (unsigned char) n;
	}
	return in;
}

MacAddress MacAddress::fromString(const char *str)
{
	std::istringstream stream(str);
	MacAddress addr;
	stream >> addr;
	return addr;
}

