
PROGS=mcast arpd qhandler
ARPD_OBJS=arpd.o macaddress.o ipaddress.o
QHANDLER_OBJS=qhandler.o ipaddress.o
ALLHEADERS=fnv.h ipaddress.h macaddress.h
ALLDEPS=Makefile $(ALLHEADERS)

all: $(PROGS)
clean: 
	rm -f $(PROGS) $(ARPD_OBJS)

arpd: $(ARPD_OBJS) $(ALLDEPS)
	g++ -o $@ $(ARPD_OBJS)

qhandler: $(QHANDLER_OBJS) $(ALLDEPS)
	g++ -o $@ $(QHANDLER_OBJS) -lnetfilter_queue

LDFLAGS=
CFLAGS=-Wall -O
CPPFLAGS=-Wall -g

%.o: %.cpp $(ALLDEPS)
	g++ $(CPPFLAGS) -c -o $@ $< 
	
