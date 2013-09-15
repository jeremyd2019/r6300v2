#!/bin/sh

export PATH=/projects/hnd/tools/linux/hndtools-arm-linux-2.6.36-uclibc-4.5.3/bin:$PATH
#export
#KERNEL_DIR=/home/perry/project/WNDR4500/2011_04_14_WNDR4500_Alpha/src/linux/linux-2.6

export TARGETDIR="../../../src/router"
export TOP="/home/perry/project/WNDR4500/2011_04_14_WNDR4500_Alpha/src/router"

export CROSS_COMPILE="arm-uclibc-linux-2.6.36"
export CPPFLAGS="-I\$(TOP)../../ap/gpl/openssl/tmp/usr/local/ssl/include"
export LDFLAGS="-L\$(TOP)../../ap/gpl/openssl/tmp/usr/local/ssl/lib"
export PKG_CONFIG_PATH="\$(TOP)../../ap/gpl/openssl/tmp/usr/local/ssl/lib/pkgconfig"
#export CC="mipsel-uclibc-linux26-gcc"
#export AR="mipsel-uclibc-linux26-ar"
#export AS="mipsel-uclibc-linux26-as"
#export LD="mipsel-uclibc-linux26-ld"
#export NM="mipsel-uclibc-linux26-nm"
#export RANLIB="mipsel-uclibc-linux26-ranlib"
export bindir="/BIN"


#CPPFLAGS="-I/path/to/ssl/include" LDFLAGS="-L/path/to/ssl/lib"

# for curl
#cd curl-7.23.1
./configure --target=arm-linux --host=arm-linux --build=i386-pc-linux-gnu --prefix='$(shell pwd)/tmp' --with-ssl=$TOP../../ap/gpl/openssl/tmp/usr/local/ssl \
            --exec-prefix='$(TARGETDIR)' --disable-manual --disable-proxy --without-zlib --disable-cookies --disable-ipv6 --disable-shared \
            --disable-dict --disable-file --disable-ftp --disable-gopher --disable-imap --disable-pop3 \
            --disable-smtp --disable-telnet --disable-tftp --disable-rtsp  
#make
#make install
#cd -
