
KERNEL_HEADERS=-I$(LINUXDIR)/include

CC=$(CROSS_COMPILE)gcc
CFLAGS= -Wall -g -O2
LDFLAGS=
INCLUDE=-I../libbridge $(KERNEL_HEADERS) 
LIBS= -L ../libbridge -lbridge 

prefix=/usr
exec_prefix=${prefix}
bindir=${exec_prefix}/bin
sbindir=${exec_prefix}/sbin
mandir=${prefix}/man

INSTALL=/tools/bin/install -c


common_SOURCES= brctl_cmd.c brctl_disp.c 
brctl_SOURCES=  brctl.c $(common_SOURCES)

common_OBJECTS= $(common_SOURCES:.c=.o)
brctl_OBJECTS= $(brctl_SOURCES:.c=.o)

OBJECTS= $(common_OBJECTS) $(brctl_OBJECTS)

PROGRAMS= brctl 


all:	$(PROGRAMS)

install: $(PROGRAMS)
	mkdir -p $(DESTDIR)$(sbindir)
	$(INSTALL) -m 755 $(PROGRAMS) $(DESTDIR)$(sbindir)

brctl:	$(brctl_OBJECTS) ../libbridge/libbridge.a
	$(CC) $(LDFLAGS) $(brctl_OBJECTS) $(LIBS) -o brctl

%.o: %.c brctl.h
	$(CC) $(CFLAGS) $(INCLUDE) -c $< 

clean:
	rm -f *.o brctl core
