#
# Copyright (C) 2010, Broadcom Corporation
# All Rights Reserved.
# 
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#
# $Id: Makefile,v 1.17 2009/04/14 15:49:12 Exp $
#
ifeq "$(SRCBASE)" ""
	SRCBASE=$(shell cd ../../../.. ; pwd)
endif

include $(SRCBASE)/router/.config

CFG_BCMIGD ?= 1

UPNP = $(shell (cd ../.. && pwd -P))
UPNPBASE = $(UPNP)/upnp
UPNPDEV = $(UPNP)/device
OBJDIR = obj

#
# conditional objects
#
CFLAGS += -Wall -Wunused -g -O2


INCS = . $(UPNPDEV) $(UPNP)/include $(SRCBASE)/include $(SRCBASE)/router/shared $(SRCBASE)/include/bcmcrypto

#
# Common sources
#
SOURCES += upnp.c ssdp.c upnp_http.c gena.c soap.c \
	upnp_description.c upnp_device.c upnp_req.c upnp_util.c

SOURCES += device_init.c

SOURCES += linux_main.c linux_osl.c

SOURCES += UdpLib.c

#
# IGD
#
ifeq ($(CONFIG_NAT),y)
ifeq ($(strip $(CFG_BCMIGD)),1)
CFLAGS  += -D__BCMIGD__
INCS += $(UPNPDEV)/InternetGatewayDevice
SOURCES += InternetGatewayDevice_table.c \
	InternetGatewayDevice.c \
	soap_x_layer3forwarding.c \
	soap_x_wancommoninterfaceconfig.c \
	soap_x_wanipconnection.c \
	xml_InternetGatewayDevice.c \
	xml_x_layer3forwarding.c \
	xml_x_wancommoninterfaceconfig.c \
	xml_x_wanipconnection.c
endif
endif

#
# WFA
#
ifeq ($(CONFIG_WSCCMD),y)
INCS += $(UPNPDEV)/WFADevice $(SRCBASE)/wps/common/include
SOURCES += WFADevice_table.c \
	WFADevice.c \
	soap_x_wfawlanconfig.c \
	xml_WFADevice.c \
	xml_x_wfawlanconfig.c
endif

vpath %.c .:$(UPNPBASE):$(UPNPDEV):$(UPNPDEV)/InternetGatewayDevice:$(UPNPDEV)/WFADevice:$(SRCBASE)/router/shared

INCS := $(patsubst %,-I%,$(INCS))
CFLAGS += $(INCS)

SOURCES.OBJ := $(SOURCES)
SOURCES.OBJ := $(patsubst %.c,$(OBJDIR)/%.o,$(SOURCES.OBJ))

LIBS = -L$(TOP)/libbcmcrypto -L$(SRCBASE)/router/nvram -L$(SRCBASE)/router/shared \
	-lbcmcrypto -lnvram -lshared

#
# Rules
#
TARGET = upnp

all : $(OBJDIR) $(TARGET)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)/%.o:	%.c
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ -c $<


$(TARGET) :: $(SOURCES.OBJ)
	$(CC) -o $@ $(SOURCES.OBJ) $(LIBS)
	$(SIZE)  $(TARGET)
ifneq ($(DEBUG),1)
	$(STRIP) $@
endif

clean: 
	rm -rf obj 

install:
	install -d $(INSTALLDIR)/usr/sbin
	install -m 755 upnp $(INSTALLDIR)/usr/sbin/upnp
	$(STRIP) $(INSTALLDIR)/usr/sbin/upnp

FORCE:

.PHONY: FORCE clean libs install
