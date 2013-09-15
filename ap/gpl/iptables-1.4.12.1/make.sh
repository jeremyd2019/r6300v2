#!/bin/sh

export PATH=/projects/hnd/tools/linux/hndtools-mipsel-linux-uclibc-4.2.3/bin:$PATH
export KERNEL_DIR="../../../src/linux/linux-2.6"

export CROSS_COMPILE="mipsel-uclibc-linux26-"
export CPPFLAGES=$PWD/tmp/include
export CC="mipsel-uclibc-linux26-gcc"
export AR="mipsel-uclibc-linux26-ar"
export AS="mipsel-uclibc-linux26-as"
export LD="mipsel-uclibc-linux26-ld"
export NM="mipsel-uclibc-linux26-nm"
export RANLIB="mipsel-uclibc-linux26-ranlib"

# for iptables
#cd iptables-1.4.12.1
./configure --prefix=$PWD/tmp --host=mipsel-linux --with-ksource=$KERNEL_DIR --disable-shared #--without-kernel
#--without-kernel
#make
#make install
#cd -
