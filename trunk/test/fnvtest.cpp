#include "fnv.h"
#include <cstring>
#include <iostream>

int main(int argc, char *argv[])
{
	if (argc > 1) {
		int len = std::strlen(argv[1]);
		FnvHash h;
		h.addData((unsigned char *) argv[1], len);
		std::cout << "hash=" << h.hash << std::endl;
		std::cout << "16bit=" << h.get16() << std::endl;
		std::cout << "rev=" << h.get32rev() << std::endl;
		std::cout << "16rev=" << h.get16rev() << std::endl;
	}
	return 0;
}

