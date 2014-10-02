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

#SUBDIRS +=openssl
#SUBDIRS +=lzo-2.06
ifeq ($(CONFIG_OPENVPN),y)
SUBDIRS +=openvpn-2.3.1
#SUBDIRS +=GeoIP-1.5.1
endif
# allenwen add for openvpn

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
#SUBDIRS	+= samba-3.0.13
SUBDIRS	+= samba-3.0.25b
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

ifeq ($(CONFIG_TIMEMACHINE),y)
SUBDIRS += timemachine
endif
ifeq ($(CONFIG_WGET),y)
OPENSSL_DIR = $(GPLTOPDIR)/openssl
WGET_DIR = wget-1.12
SUBDIRS += $(WGET_DIR)
endif

SUBDIRS += zip30 unzip60
all: ECOSYSTEM config
	for i in ${SUBDIRS}; do \
		(cd $$i; make) || exit 1; \
	done

ECOSYSTEM:
ifeq ($(CONFIG_WGET),y)
	mkdir -p $(OPENSSL_DIR)/lib && rm -rf $(OPENSSL_DIR)/lib/libssl.so.1.0.0 && cp $(TOOLCHAIN)/usr/lib/libssl.so.1.0.0 $(OPENSSL_DIR)/lib/
	cd $(WGET_DIR) && $(CONFIGURE) CFLAGS="$(CFLAGS) -DMB_CUR_MAX=1" LDFLAGS="$(LDFLAGS) -L$(OPENSSL_DIR)" \
		--with-ssl=openssl --with-libssl-prefix=$(OPENSSL_DIR) --prefix=$(TARGETDIR)
endif
	# Foxconn add end by Hank for ecosystem support 08/14/2012	
config: GeoIP_config openvpn_config
GeoIP_config:
	if [ ! -f ./GeoIP-1.5.1/Makefile ];then \
	cd GeoIP-1.5.1 && (CC=$(CROSS_COMPILE)gcc \
	ac_cv_func_malloc_0_nonnull=yes ac_cv_func_realloc_0_nonnull=yes CFLAGS="$(CFLAGS)" \
	$(CONFIGURE) --with-gnu-ld --prefix=$(TARGETDIR)/usr); \
	fi

lzo_config:
	if [ ! -f ./lzo-2.06/Makefile ];then \
	cd lzo-2.06 && (CC=$(CROSS_COMPILE)gcc CFLAGS="$(CFLAGS)" \
	$(CONFIGURE) --prefix=$(TARGETDIR)/usr/local); \
	fi

lzo_make: lzo_config
	cd lzo-2.06 && make install

openvpn_config: lzo_make
	if [ ! -f ./openvpn-2.3.1/Makefile ];then \
	cd openvpn-2.3.1 && (CC=$(CROSS_COMPILE)gcc LZO_CFLAGS=-I$(TARGETDIR)/usr/local/include \
	LZO_LIBS="-L$(TARGETDIR)/usr/local/lib -llzo2" \
	CFLAGS="$(CFLAGS) -I$(SRCBASE)/../ap/acos/include" \
	LDFLAGS="-L$(ROUTERDIR)/nvram -L$(INSTALLDIR)/nvram/usr/lib -lnvram" \
	$(CONFIGURE) --prefix=$(TARGETDIR)/usr/local --disable-pam-dlopen \
	--disable-plugin-auth-pam); \
	fi 

install:
	for i in ${SUBDIRS}; do \
		(cd $$i; make install) || exit 1; \
	done
	
clean:
	for i in ${SUBDIRS}; do \
		(cd $$i; make clean); \
	done

.PHONY: $(SUBDIRS)
