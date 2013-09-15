

export CROSS_COMPILE = "arm-uclibc-linux-2.6.36-"

#export PATH +=/projects/hnd/tools/linux/hndtools-mipsel-linux-uclibc-4.2.3/bin

export CPPFLAGS += -I$(TOP)/../../ap/gpl/openssl/tmp/usr/local/ssl/include -I$(TOP)/../../ap/gpl/openssl/tmp/usr/local/ssl/include/openssl
export LDFLAGS += -L$(TOP)/../../ap/gpl/openssl/tmp/usr/local/ssl/lib
export PKG_CONFIG_PATH += $(TOP)/../../ap/gpl/openssl/tmp/usr/local/ssl/lib/pkgconfig

#export CC = "mipsel-uclibc-linux26-gcc"
##export AR = "mipsel-uclibc-linux26-ar"
#export AS = "mipsel-uclibc-linux26-as"
#export LD = "mipsel-uclibc-linux26-ld"
#export NM = "mipsel-uclibc-linux26-nm"
#export RANLIB = "mipsel-uclibc-linux26-ranlib"


all: .conf  makecurl
.conf:
	cd source ; \
	./configure 	--target=arm-linux \
			--host=arm-linux \
			--build=$(BUILD) \
			--prefix='$(shell pwd)/tmp' \
			--with-ssl \
            		--exec-prefix='$(TARGETDIR)' \
			--disable-manual \
			--disable-proxy \
			--without-zlib \
			--disable-cookies \
			--disable-ipv6 \
			--disable-shared \
            		--disable-dict \
			--disable-file \
			--disable-ftp \
			--disable-gopher \
			--disable-imap \
			--disable-pop3 \
            		--disable-smtp \
			--disable-telnet \
			--disable-tftp \
			--disable-rtsp  

	touch .conf
makecurl:
	cd source ; make

clean: .conf cleancurl
	
cleancurl: 
	cd source ; make clean	

distclean:
	rm -rf .conf
	cd source ; make clean

install:
	install -d $(TARGETDIR)/sbin/
	install -m 755 $(TOP)/../../ap/gpl/curl-7.23.1/source/src/curl  $(TARGETDIR)/sbin
	$(STRIP) $(TARGETDIR)/sbin/curl
	
