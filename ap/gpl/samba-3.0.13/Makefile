all: .conf makesmb
.conf:
	cd source ; \
	./configure --without-python \
	            --with-included-popt \
	            --without-acl-support \
	            --without-spinlocks \
	            --without-profiling-data \
	            --without-syslog \
	            --without-nisplus-home \
	            --without-ldapsam \
	            --without-pam_smbpass \
	            --without-pam \
	            --without-smbmount \
	            --without-automount \
	            --without-expsam \
	            --without-krb5 \
	            --without-dce-dfs \
	            --without-vfs-afsacl \
	            --without-fake-kaserver \
	            --without-afs \
	            --without-smbwrapper \
	            --without-swatdir \
	            --without-fhs \
	            --disable-xmltest \
	            --disable-dmalloc \
	            --disable-krb5developer \
	            --disable-developer \
	            --disable-debug \
	            --without-shared-modules \
	            --without-readline \
	            --without-utmp \
	            --without-quotas \
	            --without-sys-quotas \
	            --without-ads \
	            --disable-cups \
	            --without-libsmbclient \
	            --without-smbmount \
	            --without-ldap \
	            --target=arm-linux \
	            --host=i686-linux \
	            --without-winbind \
	            --prefix=/usr/local/samba 

	touch .conf
makesmb:
	cp config_arm.h source/include/config.h
	cd source ; make

clean: .conf cleansmb
	
cleansmb: 
	cd source ; make clean	

distclean:
	rm -rf .conf
	cd source ; make distclean

install:
	install -d $(TARGETDIR)/usr/local/
	install -d $(TARGETDIR)/usr/local/samba
	install -d $(TARGETDIR)/usr/local/samba/lib
	install -d $(TARGETDIR)/tmp/samba/
	install -d $(TARGETDIR)/tmp/samba/private
	install -d $(TARGETDIR)/etc
	install -m 755 data/group $(TARGETDIR)/etc
	#install -m 755 data/smb.conf $(TARGETDIR)/usr/local/samba/lib
	install -m 755 data/lmhosts $(TARGETDIR)/usr/local/samba/lib
	install -m 755 source/bin/smb_pass  $(TARGETDIR)/usr/local/samba/
	install -m 755 source/bin/nmbd  $(TARGETDIR)/usr/local/samba/
	install -m 755 source/bin/smbd  $(TARGETDIR)/usr/local/samba/
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
	#
	#	dummy install for smbd printer part	
	#
	#cp ../LPRng-3.8.32/printcap ./
	#install -m 755 printcap $(TARGETDIR)/etc
	cd $(TARGETDIR)/etc && unlink passwd || pwd
	#cd $(TARGETDIR)/etc && ln -sf ../var/samba/private/passwd passwd
	cd $(TARGETDIR)/etc && ln -sf ../tmp/samba/private/passwd passwd
	
