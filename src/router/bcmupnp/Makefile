#
# Linux upnp Makefile support WFA device
#
# Copyright (C) 2010, Broadcom Corporation. All Rights Reserved.
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
# $Id: Makefile,v 1.10 2010/03/04 08:57:26 Exp $
#

ifeq "$(SRCBASE)" ""
	SRCBASE=$(shell cd ../.. ; pwd)
endif

include $(SRCBASE)/router/.config

ifeq ($(CONFIG_IGD),y)
CFG_BCMIGD = 0
else
CFG_BCMIGD ?= 1
endif

UPNP	:= $(SRCBASE)/router/bcmupnp/upnp/linux
CFLAGS += -s -O2 -include bcmconfig.h
#DEBUG	:= 1

ifneq ($(wildcard ./prebuilt),)
all: docopy upnp

docopy:
	install -d $(INSTALLDIR)/usr/sbin
	install -m 755 prebuilt/upnp $(INSTALLDIR)/usr/sbin/upnp
else
all: upnp
endif

install: all
	[ ! -f $(UPNP)/Makefile ] || $(MAKE) -C $(UPNP) CFG_BCMIGD=$(CFG_BCMIGD) install
	$(MAKE) -C $(UPNP) -f Makefile.nat CFG_BCMIGD=$(CFG_BCMIGD) install

clean:
	[ ! -f $(UPNP)/Makefile ] || $(MAKE) -C $(UPNP) clean
	$(MAKE) -C $(UPNP) -f Makefile.nat clean

upnp: FORCE
	[ ! -f $(UPNP)/Makefile ] || $(MAKE) -C $(UPNP) DEBUG=$(DEBUG) CFG_BCMIGD=$(CFG_BCMIGD)
	$(MAKE) -C $(UPNP) -f Makefile.nat DEBUG=$(DEBUG) CFG_BCMIGD=$(CFG_BCMIGD)

FORCE:
