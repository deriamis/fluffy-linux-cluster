
#include <time.h>
// Return the difference between two times in milliseconds
// If too big, return a big number instead.
inline int timevalSub(struct timeval & first, struct timeval & second)
{
	int secsdiff = first.tv_sec - second.tv_sec;
	if (secsdiff > 10000) {
		secsdiff = 10000;
	}
	if (secsdiff < -10000) {
		secsdiff = -10000;
	}
	int usdiff = first.tv_usec - second.tv_usec;
	return (secsdiff * 1000) + (usdiff / 1000);
}
