
DESTDIR = $(TARGETDIR)/usr/sbin

SRC = src

all : 
	@cd $(SRC); $(MAKE) dnsmasq 

clean :
	rm -f *~ contrib/*/*~ */*~ $(SRC)/*.o $(SRC)/dnsmasq core build

install : all
	install -d $(DESTDIR)
	install -m 755 $(SRC)/dnsmasq $(DESTDIR)
	$(STRIP) $(DESTDIR)/dnsmasq




