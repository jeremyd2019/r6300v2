#
# Linux router shared code Makefile
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
# $Id: Makefile 341899 2012-06-29 04:06:38Z $
#

include $(SRCBASE)/router/.config

CFLAGS	+= -I. -I../httpd -I$(SRCBASE)/include -Wall -fPIC
CFLAGS	+= $(WLAN_ComponentIncPath)
ifneq (2_4,$(LINUX_VERSION))
CFLAGS += -I$(LINUXDIR)/include
CFLAGS += -DLINUX26
endif
#CFLAGS	+= -g -DDEBUG
CFLAGS	+= -s -O2
ifeq ($(CONFIG_RTR_OPTIMIZE_SIZE),y)
CFLAGS  += -Os
else
CFLAGS  += -O2
endif
LDFLAGS += -L.


OBJS := nvparse.o shutils.o wl.o wl_linux.o linux_timer.o defaults.o wlif_utils.o

ifneq ($(CONFIG_WAPI)$(CONFIG_CIFS),)
OBJS += confmtd_utils.o
endif

ifeq ($(CONFIG_PLC),y)
OBJS += dict.o
endif

all: libshared.so

install: all
	install -d $(INSTALLDIR)/usr/lib
	install -m 755 libshared.so $(INSTALLDIR)/usr/lib
	$(STRIP) $(INSTALLDIR)/usr/lib/libshared.so

clean:
	rm -f *.o *.so

libshared.so: $(OBJS)
	$(LD) -shared -o $@ $^
