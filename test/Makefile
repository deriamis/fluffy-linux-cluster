
PROGS=arpd fluffycluster
ARPD_OBJS=arpd.o macaddress.o ipaddress.o
FLUFFYCLUSTER_OBJS=fluffycluster.o ipaddress.o macaddress.o \
    ifholder.o membership.o qhandler.o utils.o systemsetup.o

ALLHEADERS=fnv.h ipaddress.h macaddress.h qhandler.h  \
	ifholder.h membership.h utils.h timing.h systemsetup.h
ALLDEPS=Makefile $(ALLHEADERS)

all: $(PROGS)
clean: 
	rm -f $(PROGS) $(ARPD_OBJS) $(FLUFFYCLUSTER_OBJS)

arpd: $(ARPD_OBJS) $(ALLDEPS)
	g++ -o $@ $(ARPD_OBJS)

fluffycluster: $(FLUFFYCLUSTER_OBJS) $(ALLDEPS)
	g++ -o $@ $(FLUFFYCLUSTER_OBJS) -lnetfilter_queue
	
LDFLAGS=
CFLAGS=-Wall -O
CPPFLAGS=-Wall -g

%.o: %.cpp $(ALLDEPS)
	g++ $(CPPFLAGS) -c -o $@ $< 
	
