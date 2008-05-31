#ifndef UTILS_INCLUDED
 #define UTILS_INCLUDED

#include <string>
#include <sstream>

void SetNonBlocking(int sock);

template <typename T> std::string toString(T obj)
{
	std::ostringstream oss;
	oss << obj;
	return oss.str();
}

#endif // UTILS_INCLUDED
