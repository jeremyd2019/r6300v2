#
# Layer Two Tunneling Protocol Daemon
# Copyright (C)1998 Adtran, Inc.
#
# Mark Spencer <markster@marko.net>
#
# This is free software.  You may distribute it under
# the terms of the GNU General Public License,
# version 2, or at your option any later version.
#
# Note on debugging flags:
# -DDEBUG_ZLB shows all ZLB exchange traffic
# -DDEBUG_CONTROL debugs the /var/run/l2tp-control pipe interface
# -DDEBUG_HELLO debugs when hello messages are sent
# -DDEBUG_CLOSE debugs call and tunnel closing
# -DDEBUG_FLOW debugs flow control system
# -DDEBUG_FILE debugs file format
# -DDEBUG_AAA debugs authentication, accounting, and access control
# -DDEBUG_PAYLOAD shows info on every payload packet
# -DDEBUG_CONTROL shows info on every control packet
# -DDEBUG_PPPD shows the command line of pppd
# -DDEBUG_HIDDEN debugs hidden AVP's
# -DDEBUG_ENTROPY debug entropy generation
# -DTEST_HIDDEN makes Assigned Call ID sent as a hidden AVP
#
# Also look at the top of network.c for some other (eventually to 
# become runtime options) debugging flags
#
include ../config.in
include ../config.mk

#CC= /opt/brcm/hndtools-mipsel-uclibc-3.2.3/bin/mipsel-uclibc-gcc
#DFLAGS= -g -O2 -DDEBUG_PPPD
DFLAGS= -g -O2 -DDEBUG_PPPD -DDEBUG_CONTROL -DDEBUG_ENTROPY -DDEBUG_CLOSE -DPPPOX_L2TP
#
# Uncomment the next line for Linux
#
#OSFLAGS= -DLINUX -I/usr/include

ifeq ($(CONFIG_KERNEL_2_6_36),y)
OSFLAGS= -DLINUX -I$(TOOLCHAIN)/include/ -DUSE_KERNEL
else
OSFLAGS= -DLINUX -I/opt/brcm/hndtools-mipsel-uclibc-3.2.3/include/
endif


OSFLAGS+= -I$(LINUXDIR)/include/

#
# Uncomment the following to use the kernel interface under Linux
#
#OSFLAGS+= -DUSE_KERNEL
#
# Uncomment the next line for FreeBSD
#
#OSFLAGS= -DFREEBSD
#
# Uncomment the next line for Solaris. For solaris, at least,
# we don't want to specify -I/usr/include because it is in
# the basic search path, and will over-ride some gcc-specific
# include paths and cause problems.
#
#OSFLAGS= -DSOLARIS
#OSLIBS= -lnsl -lsocket
#
# Feature flags
#
# Comment the following line to disable l2tpd maintaining IP address
# pools to pass to pppd to control IP address allocation

FFLAGS= -DIP_ALLOCATION 

CFLAGS= $(DFLAGS) -Wall -DSANITY $(OSFLAGS) $(FFLAGS)

#add static_pppoe define.
ifeq ($(CONFIG_STATIC_PPPOE),y)
CFLAGS  += -DSTATIC_PPPOE
else
CFLAGS  += -USTATIC_PPPOE
endif

HDRS=l2tp.h avp.h misc.h control.h call.h scheduler.h file.h aaa.h md5.h
OBJS=l2tpd.o pty.o misc.o control.o avp.o call.o network.o avpsend.o scheduler.o file.o aaa.o md5.o
LIBS= $(OSLIB) # -lefence # efence for malloc checking
BIN=l2tpd
BINDIR=$(TARGETDIR)/usr/sbin
ETCDIR=$(TARGETDIR)/etc

all: $(BIN)

clean:
	rm -f $(OBJS) $(BIN)

$(BIN): $(OBJS) $(HDRS)
	$(CC) -o $(BIN) $(DFLAGS) $(OBJS) $(LIBS)

install:
	mkdir -p $(BINDIR)
	install -m 755 l2tpd $(BINDIR)
	$(STRIP) $(BINDIR)/l2tpd
	rm -f $(BINDIR)/st*
