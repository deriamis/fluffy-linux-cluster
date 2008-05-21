
PROGS=mcast arpd fluffycluster
ARPD_OBJS=arpd.o macaddress.o ipaddress.o
MCAST_OBJS=mcast.o ipaddress.o
FLUFFYCLUSTER_OBJS=fluffycluster.o ipaddress.o macaddress.o \
    ifholder.o membership.o qhandler.o utils.o

ALLHEADERS=fnv.h ipaddress.h macaddress.h qhandler.h  \
	ifholder.h membership.h utils.h
ALLDEPS=Makefile $(ALLHEADERS)

all: $(PROGS)
clean: 
	rm -f $(PROGS) $(ARPD_OBJS)

arpd: $(ARPD_OBJS) $(ALLDEPS)
	g++ -o $@ $(ARPD_OBJS)

mcast: $(MCAST_OBJS) $(ALLDEPS)
	g++ -o $@ $(MCAST_OBJS)

fluffycluster: $(FLUFFYCLUSTER_OBJS) $(ALLDEPS)
	g++ -o $@ $(FLUFFYCLUSTER_OBJS) -lnetfilter_queue
	
LDFLAGS=
CFLAGS=-Wall -O
CPPFLAGS=-Wall -g

%.o: %.cpp $(ALLDEPS)
	g++ $(CPPFLAGS) -c -o $@ $< 
	
