#
# vmstat utility Makefile
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
# $Id: Makefile 321517 2012-03-15 17:15:37Z $
#

CFLAGS	+= -I. -I$(TOP)/shared -I$(SRCBASE)/include -Wall
#CFLAGS	+= -g -DDEBUG
CFLAGS	+= -s -O2


OBJS := vmstat.o proc/sysinfo.o 

all: vmstat

clean:
	rm -f *.o proc/*.o vmstat 

install: all
	install -d $(INSTALLDIR)/usr/bin
	install vmstat $(INSTALLDIR)/usr/bin
	$(STRIP) $(INSTALLDIR)/usr/bin/vmstat

vmstat: $(OBJS)
	$(CC) -o $@ $^
