#
# IPTABLES Makefile
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
# $Id: $
#

include ../.config

HERE := $(shell pwd)
IPTABLES_SRC = src
IPTLIB_DIR=$(IPTABLES_SRC)/libiptc/.libs


DIRS := ${IPTABLES_SRC}
IPT_INSTALL_DIR=$(INSTALLDIR)/usr


ifeq ($(CONFIG_IPV6),y)
DOIPV6=1
else
DOIPV6=0
endif


.PHONY: all
all: clean iptables

.PHONY: configure
ifeq ($(LINUX_VERSION),2_6_36)
ifeq ($(CONFIG_IPV6),y)
configure:
	[ -f $(IPTABLES_SRC)/Makefile ] || \
	(cd $(IPTABLES_SRC); \
	 CC=$(CC) ./configure \
	    --target=$(PLATFORM)-linux \
	    --host=$(PLATFORM)-linux \
	    --build=`/bin/arch`-linux \
	    --with-kernel=$(KERNEL_DIR) \
	    --prefix=$(HERE)/$(IPTABLES_SRC)/install \
	)
else # disable ipv6
configure:
	[ -f $(IPTABLES_SRC)/Makefile ] || \
	(cd $(IPTABLES_SRC); \
	 CC=$(CC) ./configure \
	    --target=$(PLATFORM)-linux \
	    --host=$(PLATFORM)-linux \
	    --build=`/bin/arch`-linux \
	    --with-kernel=$(KERNEL_DIR) \
	    --prefix=$(HERE)/$(IPTABLES_SRC)/install \
	    --disable-ipv6 \
	)
endif
endif # linux-2.6.36

.PHONY: iptables
iptables: configure
	+$(MAKE) -C $(IPTABLES_SRC)
	+$(MAKE) -C $(IPTABLES_SRC) install
ifeq ($(CONFIG_IPV6),y)	
	$(AR) cru libiptc.a $(IPTLIB_DIR)/libip4tc.o $(IPTLIB_DIR)/libip6tc.o
else
	$(AR) cru libiptc.a $(IPTLIB_DIR)/libip4tc.o
endif

.PHONY: install
install: all
	install -d $(IPT_INSTALL_DIR)
	install -d $(IPT_INSTALL_DIR)/lib/iptables
	install $(IPTABLES_SRC)/extensions/*.so $(IPT_INSTALL_DIR)/lib/iptables
	$(STRIP) $(IPT_INSTALL_DIR)/lib/iptables/*.so
	cp -rf $(IPTABLES_SRC)/install/sbin $(IPT_INSTALL_DIR)
	install -d $(INSTALLDIR)/lib
	cp -f $(IPTABLES_SRC)/install/lib/libip* $(INSTALLDIR)/lib


.PHONY: clean
clean: 
	+$(if $(wildcard $(IPTABLES_SRC)/Makefile), \
		$(MAKE) -C $(IPTABLES_SRC) distclean \
	   , \
	    @true \
	  )
	  rm -f libiptc.a

.PHONY: $(addprefix clean-,${DIRS})
$(addprefix clean-,${DIRS}):
	+$(MAKE) -C $(patsubst clean-%,%,$@) clean
