#
# FLOW userspace library
#
# Copyright (C) 2013, Broadcom Corporation. All Rights Reserved.
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
# $Id: $
#
include $(TOP)/.config

CTF_INCLUDES := -I$(SRCBASE)/include -I$(SRCBASE)/include/ctf

NETFILTER_INCLUDES := $(LIBNETFILTER_CONNTRACK_CFLAGS) $(LIBMNL_CFLAGS) $(LIBNFNETLINK_CFLAGS)
NETFILTER_LIBS := $(LIBNETFILTER_CONNTRACK_LIBS) $(LIBMNL_LIBS) $(LIBNFNETLINK_LIBS)

CCFLAGS = -Os -Wall $(CTF_INCLUDES)

ALLOBJS += flow_linux.o flow_linux_netfilter.o flow_api.o

all: example
	@echo "FLOW userspace objects built"

clean: example-clean
	rm -f libflow.so $(ALLOBJS) .*.depend

install:
	@echo "Not implemented"

libflow.so: $(ALLOBJS)
	$(LD) -shared -o $@ $(ALLOBJS)

example: libflow.so
	$(MAKE) NETFILTER_LIBS="$(NETFILTER_LIBS)" -C example all

example-clean:
	$(MAKE) -C example clean

flow_linux_netfilter.o: CCFLAGS += $(NETFILTER_INCLUDES)

%.o: %.c
	@echo " "
	@echo "Making $<"
	$(CC) $(CCFLAGS) -c $<

%.o: .%.depend

.flow_linux_netfilter.depend: CCFLAGS += $(NETFILTER_INCLUDES)

.%.depend: %.c
	$(CC) $(CCFLAGS) -M $< > $@

.PHONY: example

-include $(ALLOBJS:%.o=.%.depend)
