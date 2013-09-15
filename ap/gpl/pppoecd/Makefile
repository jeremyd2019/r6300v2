#uncomment the following line to add checks for buggy ACs which send out
#duplicate packets

include ../config.mk
include ../config.in

#DEFINES= -DBUGGY_AC
#use the following line for OpenBSD support
#DEFINES= -DUSE_BPF
DEFINES=

#CFLAGS= -Wall -pedantic -ansi -g $(DEFINES)
CFLAGS= -Wall -pedantic -ansi -O2 $(DEFINES)

ifeq ($(MPOE_ENABLE_FLAG),y)
CFLAGS += -DMULTIPLE_PPPOE
endif

ifeq ($(CONFIG_NEW_WANDETECT),y)
CFLAGS += -DNEW_WANDETECT
endif

#Linux support doesn't need extra libraries, but OpenBSD support
#does.  If using OpenBSD, uncomment the following line:
#LIBS=-lkvm


VERSION= 0.3

ifeq ($(CONFIG_SINGLE_PROCESS_PPPOE),y)
pppoecd: pppoe2.o
	$(CC) -o pppoecd pppoe2.o $(LIBS)
else
pppoecd: pppoe.o
	$(CC) -o pppoecd pppoe.o $(LIBS)
endif

all: pppoecd

install: all
	install -d $(INSTALLDIR)/usr/sbin
	install -m 755 pppoecd $(INSTALLDIR)/usr/sbin
	$(STRIP) $(INSTALLDIR)/usr/sbin/pppoecd
ifeq ($(INCLUDE_IPV6_FLAG),y)
	cd $(INSTALLDIR)/usr/sbin && rm -f pppoecdv6 && ln -s pppoecd pppoecdv6
endif

clean:
	rm -f *.o pppoecd

