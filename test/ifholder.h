
#include "macaddress.h"

struct InterfaceHolder {
	InterfaceHolder(int ifindex, const MacAddress & sharedmac);
	~InterfaceHolder();

	int ifindex;
	MacAddress sharedmac;
	int sock;
};
