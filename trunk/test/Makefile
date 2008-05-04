
PROGS=mcast arpd
ARPD_OBJS=arpd.o macaddress.o

all: $(PROGS)
clean: 
	rm -f $(PROGS)

arpd: $(ARPD_OBJS)
	g++ -o $@ $(ARPD_OBJS)

LDFLAGS=
CFLAGS=-Wall -O
CPPFLAGS=-Wall -g
