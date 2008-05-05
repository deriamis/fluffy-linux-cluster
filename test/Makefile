
PROGS=mcast arpd
ARPD_OBJS=arpd.o macaddress.o ipaddress.o

all: $(PROGS)
clean: 
	rm -f $(PROGS) $(ARPD_OBJS)

arpd: $(ARPD_OBJS)
	g++ -o $@ $(ARPD_OBJS)

LDFLAGS=
CFLAGS=-Wall -O
CPPFLAGS=-Wall -g

%.o: %.cpp $(ALLHEADERS)
	g++ $(CPPFLAGS) -c -o $@ $< 
	
