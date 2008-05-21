#include "utils.h"

#include <sys/ioctl.h>
#include <stdexcept>

void SetNonBlocking(int sock)
{
	int nonblock = 1;
	int res = ioctl(sock, FIONBIO, &nonblock);
	if (res != 0) {
		throw std::runtime_error("nonblocking");
	}
}
