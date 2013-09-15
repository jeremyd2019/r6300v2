#
# Copyright 2005  Hon Hai Precision Ind. Co. Ltd.
#  All Rights Reserved.
# No portions of this material shall be reproduced in any form without the
# written permission of Hon Hai Precision Ind. Co. Ltd.
#
# All information contained in this document is Hon Hai Precision Ind.
# Co. Ltd. company private, proprietary, and trade secret property and
# are protected by international intellectual property laws and treaties.
#
# $Id$
#

include config.mk
include config.in

#
# Paths
#
#

SUBDIRS := 

#SUBDIRS += dnsmasq
SUBDIRS += dnsmasq-2.15
SUBDIRS += udhcpd
#SUBDIRS += ppp/pppoecd
SUBDIRS += ppp-2.4.4/pppd pppoecd
SUBDIRS += ntpclient
SUBDIRS += bzip2

ifeq ($(CONFIG_RIP),y)
SUBDIRS	+= zebra
endif

ifeq ($(CONFIG_PPTP),y)
#SUBDIRS	+= ppp-2.4.4/pppd pptp
# SUBDIRS	+= ppp/pppd pptp
SUBDIRS	+= pptp
endif

ifeq ($(CONFIG_L2TP),y)
SUBDIRS += l2tpd-0.69
endif

#ifeq ($(CONFIG_BPA),y)
#SUBDIRS	+= bpalogin
#endif

ifeq ($(CONFIG_DLNA),y)
SUBDIRS += minidlna
endif

SUBDIRS += zeroconf-0.9

#water, 10/30/2008, @samba
ifeq ($(SAMBA_ENABLE_FLAG),y)
SUBDIRS	+= samba-3.0.13
SUBDIRS	+= bftpd-1.6.6
endif

# pling added 05/04/2009, for NTFS-3g
ifeq ($(CONFIG_NTFS_3G),y)
SUBDIRS += ntfs-3g-2009.3.8
endif
#pling added 05/07/2009, for mtools (to read FAT volume label)
ifeq ($(CONFIG_MTOOLS),y)
SUBDIRS += mtools-4.0.10
endif

ifeq ($(CONFIG_IGMP_PROXY),y)
SUBDIRS	+= IGMP-PROXY
endif

SUBDIRS += iproute2

ifeq ($(CONFIG_WGET),y)
OPENSSL_DIR = $(GPLTOPDIR)/openssl
WGET_DIR = wget-1.12
SUBDIRS += $(WGET_DIR)
endif

all: ECOSYSTEM
	for i in ${SUBDIRS}; do \
		(cd $$i; make) || exit 1; \
	done

ECOSYSTEM:
ifeq ($(CONFIG_WGET),y)
	mkdir -p $(OPENSSL_DIR)/lib && cp $(OPENSSL_DIR)/libssl.so $(OPENSSL_DIR)/lib/
	cd $(WGET_DIR) && $(CONFIGURE) CFLAGS="$(CFLAGS) -DMB_CUR_MAX=1" LDFLAGS="$(LDFLAGS) -L$(OPENSSL_DIR)" \
		--with-ssl=openssl --with-libssl-prefix=$(OPENSSL_DIR) --prefix=$(TARGETDIR)
endif	

install:
	for i in ${SUBDIRS}; do \
		(cd $$i; make install) || exit 1; \
	done
	
clean:
	for i in ${SUBDIRS}; do \
		(cd $$i; make clean); \
	done

.PHONY: $(SUBDIRS)
