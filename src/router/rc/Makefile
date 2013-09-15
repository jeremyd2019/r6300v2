#
# Router rc control scripts Makefile
#
# Copyright (C) 2012, Broadcom Corporation. All Rights Reserved.
# 
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
# 
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
# SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
# OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
# CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#
# $Id: Makefile 360733 2012-10-04 07:54:28Z $
#

include $(TOP)/.config

ifeq ($(LINUXDIR), $(BASEDIR)/components/opensource/linux/linux-2.6.36)
CFLAGS += -DLINUX26 -DLINUX_2_6_36
CFLAGS += -DACOS_LINUX26

CFLAGS += -I. -I$(TOP)/shared -I$(SRCBASE)/include -Wall -I$(LINUXDIR)/include -I$(TOP)/busybox-1.x
else
CFLAGS += -I. -I$(TOP)/shared -I$(TOP)/busybox -I$(SRCBASE)/include -Wall
endif

CFLAGS += $(if $(WLAN_ComponentIncPath),$(WLAN_ComponentIncPath),$(addprefix -I,$(wildcard $(SRCBASE)/shared/bcmwifi/include)))
CFLAGS += -I$(TOP)/dpsta

#CFLAGS	+= -g -DDEBUG
CFLAGS	+= -s
ifeq ($(CONFIG_RTR_OPTIMIZE_SIZE),y)
CFLAGS  += -Os
else
CFLAGS  += -O2
endif
LDFLAGS	+= -L$(TOP)/nvram -L$(PLATFORMDIR)/install/nvram/usr/lib -lnvram -L$(TOP)/shared -L$(PLATFORMDIR)/install/shared/usr/lib -lshared \
	-L$(TOP)/libbcmcrypto -L$(INSTALLDIR)/libbcmcrypto/usr/lib -lbcmcrypto
LDFLAGS += -L$(TOP)/libbcm -L$(INSTALLDIR)/libbcm/usr/lib -lbcm
CFLAGS	+= -I$(SRCBASE)/../ap/acos/shared
LDFLAGS	+= -L$(SRCBASE)/../ap/acos/shared -L$(TARGETDIR)/usr/lib -lacos_shared

ifneq ($(PROFILE),)	# PROFILE eq R6300v2 || R6250
CFLAGS += -DUSE_KERNEL_NTFS
endif
LDFLAGS += $(EXTRA_LDFLAGS)


ifeq ($(BUILD_WFA_APUT),1)
CFLAGS	+= -DCAPI_AP
endif

ifeq ($(FW_TYPE),TWC)
CFLAGS += -DBUILD_TWC
endif
ifeq ($(CONFIG_WAPI_IAS),y)
CFLAGS += -DRC_BUILDTIME=$(shell date -d "now" +"%s")
endif

OBJS := rc.o init.o interface.o network.o services.o udhcpc.o http.o stats.o bcmutils.o mtd.o usb_hotplug.o pmon.o 
ifeq ($(CONFIG_BCMQOS),y)
OBJS += qos.o
endif

ifeq ($(CONFIG_NAT),y)
OBJS += firewall.o
LDFLAGS += -L$(TOP)/netconf -L$(PLATFORMDIR)/install/netconf/usr/lib -lnetconf
endif

ifeq ($(CONFIG_NAT),y)
OBJS += ppp.o
endif

ifeq ($(CONFIG_WCN),y)
OBJS += usb_hotplug.o
endif



ifneq ($(PROFILE),)  # R6300v2 || R6250
OBJS += hotplug_usb2.o
endif




vpath %.c $(TOP)/shared $(SRCBASE)/shared

all: rc

clean:
	rm -f *.o rc .*.depend

install: all
	install -d $(INSTALLDIR)/sbin
	install rc $(INSTALLDIR)/sbin
	$(STRIP) $(INSTALLDIR)/sbin/rc
	cd $(INSTALLDIR)/sbin && ln -sf rc init
	cd $(INSTALLDIR)/sbin && ln -sf rc erase
	cd $(INSTALLDIR)/sbin && ln -sf rc write
	cd $(INSTALLDIR)/sbin && ln -sf rc stats
	cd $(INSTALLDIR)/sbin && ln -sf rc hotplug
	cd $(INSTALLDIR)/sbin && ln -sf rc preinit

rc: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS) -lgcc_s

%.o: .%.depend
.%.depend: %.c
	$(CC) $(CFLAGS) -M $< > $@

-include $(OBJS:%.o=.%.depend)
