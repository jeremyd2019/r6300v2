#
# Samba Makefile
#
# $ Copyright Open Broadcom Corporation 2010 $
#
# $Id: Makefile,v 1.6 2010-07-17 02:57:34 kenlo Exp $
#

export SAMBA_TOP := $(shell pwd)

SAMBA_SRC=source

CFLAGS += -DLINUX

DIRS := ${SAMBA_SRC}

.PHONY: all
all: samba

.PHONY: configure
configure:
	[ -f $(SAMBA_SRC)/Makefile ] || \
	(cd $(SAMBA_SRC); \
	 export SMB_BUILD_CC_NEGATIVE_ENUM_VALUES=yes ; \
	 export libreplace_cv_READDIR_GETDIRENTRIES=no ; \
	 export libreplace_cv_READDIR_GETDENTS=no ; \
	 export linux_getgrouplist_ok=no ; \
	 export samba_cv_REPLACE_READDIR=no ; \
	 export samba_cv_HAVE_WRFILE_KEYTAB=yes ; \
	 export samba_cv_HAVE_KERNEL_OPLOCKS_LINUX=yes ; \
	 export samba_cv_HAVE_IFACE_IFCONF=yes ; \
	 export samba_cv_USE_SETRESUID=yes ; \
	 CC=$(CC) ./configure \
	    --target=arm-brcm-linux-uclibcgnueabi \
	    --host=arm-brcm-linux-uclibcgnueabi \
	    --build=`/bin/arch`-linux \
	    --enable-shared \
	    --disable-static \
	    --disable-cups \
	    --disable-iprint \
	    --disable-pie \
	    --disable-fam \
	    --localstatedir=/tmp/samba/lib/ \
	    --with-configdir=/usr/local/samba/lib/ \
	    --with-privatedir=/usr/local/samba/private \
	    --with-lockdir=/usr/local/samba/var/locks \
	    --with-piddir=/usr/local/samba/var/locks \
	    --without-ldap \
	    --without-sys-quotas \
	    --without-cifsmount \
	    --prefix=/usr/local/samba; \
	)

.PHONY: samba
samba: configure headers
	+$(MAKE) -C $(SAMBA_SRC)

.PHONY: headers
headers: configure
	+$(MAKE) -C $(SAMBA_SRC) headers

.PHONY: install
install: all
	install -d $(TARGETDIR)/usr/local/
	install -d $(TARGETDIR)/usr/local/samba
	install -d $(TARGETDIR)/usr/local/samba/lib
	install -d $(TARGETDIR)/tmp/samba/
	install -d $(TARGETDIR)/tmp/samba/private
	install -d $(TARGETDIR)/etc
	install -m 755 $(SAMBA_SRC)/../data/group $(TARGETDIR)/etc
	install -m 755 $(SAMBA_SRC)/../data/lmhosts $(TARGETDIR)/usr/local/samba/lib
	install -m 755 $(SAMBA_SRC)/bin/smb_pass  $(TARGETDIR)/usr/local/samba/
	install -m 755 $(SAMBA_SRC)/bin/nmbd  $(TARGETDIR)/usr/local/samba/
	install -m 755 $(SAMBA_SRC)/bin/smbd  $(TARGETDIR)/usr/local/samba/
	install -d $(TARGETDIR)/usr
	install -d $(TARGETDIR)/usr/lib
	install -m 755 $(SAMBA_SRC)/bin/libbigballofmud.so $(TARGETDIR)/usr/lib/libbigballofmud.so.0
	$(STRIP) $(TARGETDIR)/usr/local/samba/smbd
	$(STRIP) $(TARGETDIR)/usr/local/samba/nmbd
	$(STRIP) $(TARGETDIR)/usr/local/samba/smb_pass
	cd $(TARGETDIR)/usr/local/samba && unlink  private || pwd
	cd $(TARGETDIR)/usr/local/samba && unlink  var || pwd
	cd $(TARGETDIR)/usr/local/samba && unlink  lock || pwd
	cd $(TARGETDIR)/usr/local/samba && ln -sf ../../../tmp/samba/private private
	cd $(TARGETDIR)/usr/local/samba && ln -sf ../../../var var
	cd $(TARGETDIR)/usr/local/samba && ln -sf ../../../var/lock lock
	cd $(TARGETDIR)/usr/local/samba/lib && ln -sf ../../../tmp/samba/private/smb.conf smb.conf
	cd $(TARGETDIR)/etc && unlink passwd || pwd
	cd $(TARGETDIR)/etc && ln -sf ../tmp/samba/private/passwd passwd

install-%:
	+$(MAKE) -C $(patsubst install-%,%,$@) install

.PHONY: clean
clean: $(addprefix clean-,${DIRS})

.PHONY: $(addprefix clean-,${DIRS})
$(addprefix clean-,${DIRS}):
	+$(MAKE) -C $(patsubst clean-%,%,$@) clean
