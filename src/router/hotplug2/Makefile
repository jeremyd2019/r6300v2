# vim:set sw=8 nosta:

BINS=hotplug2
SUBDIRS=examples

all: $(BINS)

install:
	$(INSTALL_DIR) $(PREFIX)/hotplug2/sbin/
	$(INSTALL_BIN) $(BINS) $(PREFIX)/hotplug2/sbin/
	@for i in $(BINS); do $(STRIP) $(PREFIX)/hotplug2/sbin/$$i ; done

hotplug2: hotplug2.o childlist.o mem_utils.o rules.o
hotplug2-dnode: hotplug2-dnode.o mem_utils.o parser_utils.o


include common.mak
