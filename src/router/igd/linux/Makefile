#
# Copyright (C) 2012, Broadcom Corporation
# All Rights Reserved.
# 
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#
# $Id $
#

UPNPLIB = $(SRCBASE)/router/libupnp
SRCPATH = $(SRCBASE)/router/igd
DEVPATH = $(SRCPATH)/InternetGatewayDevice
OSLPATH = $(SRCPATH)/linux
OBJPATH = obj

INCLUDES += -I$(SRCPATH) -I$(DEVPATH) -I$(UPNPLIB)/include -I$(SRCBASE)/include -I$(SRCBASE)/router/shared

CFLAGS = $(INCLUDES)
CFLAGS += -Wall -Wunused -g -O2 -Werror


LDFLAGS = -L$(TOP)/libbcmcrypto -L$(SRCBASE)/router/nvram -L$(SRCBASE)/router/shared -L$(UPNPLIB) \
	-L$(INSTALLDIR)/nvram/usr/lib -lbcmcrypto -lnvram -lshared -lupnp
LDFLAGS += $(EXTRA_LDFLAGS)

SRCFILES = \
	igd_linux_main.c \
	igd_linux_osl.c \
	igd_mainloop.c \
	InternetGatewayDevice_table.c \
	InternetGatewayDevice.c \
	soap_x_layer3forwarding.c \
	soap_x_wancommoninterfaceconfig.c \
	soap_x_wanipconnection.c \
	xml_InternetGatewayDevice.c \
	xml_x_layer3forwarding.c \
	xml_x_wancommoninterfaceconfig.c \
	xml_x_wanipconnection.c

VPATH = $(SRCPATH):$(OSLPATH):$(DEVPATH)

OBJFILES := $(patsubst %.c,$(OBJPATH)/%.o,$(SRCFILES))

#
# Rules
#
TARGET = igd

all : $(OBJPATH) $(TARGET)

$(OBJPATH):
	mkdir -p $(OBJPATH)

$(OBJPATH)/%.o:	%.c
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ -c $<

$(TARGET) :: $(OBJFILES)
	$(CC) -o $@ $(OBJFILES) $(LDFLAGS)

install:
	install -d $(INSTALLDIR)/usr/sbin
	install -m 755 $(TARGET) $(INSTALLDIR)/usr/sbin/$(TARGET)
	$(STRIP) $(INSTALLDIR)/usr/sbin/$(TARGET)

clean:
	rm -rf $(OBJPATH)
	rm -f $(TARGET)

FORCE:

.PHONY: FORCE clean install
