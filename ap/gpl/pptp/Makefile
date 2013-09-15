# $Id: Makefile,v 1.38 2005/03/10 01:18:20 quozl Exp $

include ../config.in
include ../config.mk

VERSION=1.7.0
RELEASE=

#################################################################
# CHANGE THIS LINE to point to the location of your pppd binary.
PPPD = /usr/sbin/pppd
#################################################################

BINDIR=$(TARGETDIR)/usr/sbin
MANDIR=$(TARGETDIR)/usr/share/man/man8
PPPDIR=$(TARGETDIR)/etc/ppp

RM	= rm -f
OPTIMIZE= -O0
DEBUG	= -g
INCLUDE =
## Add undefine CODE_IN_USE compile flag on 05/12/2006, -Uxxx: Undefined, -Dxxx: Defined
#COMPILE_FLAGS = -UCODE_IN_USE
#CFLAGS  = -Wall $(OPTIMIZE) $(DEBUG) $(INCLUDE) $(COMPILE_FLAGS)
CFLAGS  = -Wall $(OPTIMIZE) $(DEBUG) $(INCLUDE)
LIBS	= -lutil
LDFLAGS	=

ifeq ($(CONFIG_STATIC_PPPOE),y)
CFLAGS  += -DSTATIC_PPPOE
else
CFLAGS  += -USTATIC_PPPOE
endif

PPTP_BIN = pptp

PPTP_OBJS = pptp.o pptp_gre.o ppp_fcs.o \
            pptp_ctrl.o dirutil.o vector.o \
            inststr.o util.o version.o \
	    pptp_quirks.o orckit_quirks.o pqueue.o pptp_callmgr.o

PPTP_DEPS = pptp_callmgr.h pptp_gre.h ppp_fcs.h util.h \
	    pptp_quirks.h orckit_quirks.h config.h pqueue.h

all: config.h $(PPTP_BIN)

$(PPTP_BIN): $(PPTP_OBJS) $(PPTP_DEPS)
	$(CC) -o $(PPTP_BIN) $(PPTP_OBJS) $(LDFLAGS) $(LIBS)

config.h: 
	echo "/* text added by Makefile target config.h */" > config.h
	echo "#define PPTP_LINUX_VERSION \"$(VERSION)$(RELEASE)\"" >> config.h
	echo "#define PPPD_BINARY \"$(PPPD)\"" >> config.h

vector_test: vector_test.o vector.o
	$(CC) -o vector_test vector_test.o vector.o

clean:
	$(RM) *.o config.h

clobber: clean
	$(RM) $(PPTP_BIN) vector_test

distclean: clobber

test: vector_test

install:
	mkdir -p $(BINDIR)
	install -m 755 pptp $(BINDIR)
#	mkdir -p $(MANDIR)
#	install -m 644 pptp.8 $(MANDIR)
	mkdir -p $(PPPDIR)
#	install -m 644 options.pptp $(PPPDIR)
	$(STRIP) $(BINDIR)/pptp
	rm -f $(BINDIR)/st*

uninstall:
	$(RM) $(BINDIR)/pptp $(MANDIR)/pptp.8

dist: clobber
	$(RM) pptp-$(VERSION)$(RELEASE).tar.gz
	$(RM) -r pptp-$(VERSION)
	mkdir pptp-$(VERSION)
	cp --recursive ChangeLog Makefile *.c *.h options.pptp pptp.8 \
		Documentation Reference AUTHORS COPYING INSTALL NEWS \
		README DEVELOPERS TODO USING \
		pptp-$(VERSION)/
	$(RM) -r pptp-$(VERSION)/CVS pptp-$(VERSION)/*/CVS
	tar czf pptp-$(VERSION)$(RELEASE).tar.gz pptp-$(VERSION)
	$(RM) -r pptp-$(VERSION)
	md5sum pptp-$(VERSION)$(RELEASE).tar.gz

deb:
	chmod +x debian/rules 
	fakeroot dpkg-buildpackage -us -uc
	mv ../pptp_$(VERSION)-0_i386.deb .

WEB=~/public_html/external/mine/pptp/pptpconfig
release:
	cp pptp_$(VERSION)-0_i386.deb $(WEB)
	cd $(WEB);make
