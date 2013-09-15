#!/bin/bash
#
# Miscellaneous steps to prepare the root filesystem
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
# $Id: rootprep.sh,v 1.16 2008-11-26 07:38:16 $
#

ROOTDIR=$PWD

# tmp
mkdir -p tmp
ln -sf tmp/var var
ln -sf tmp/media media
(cd $ROOTDIR/usr && ln -sf ../tmp)

# dev
mkdir -p dev

# etc
mkdir -p etc
echo "/lib" > etc/ld.so.conf
echo "/usr/lib" >> etc/ld.so.conf
/sbin/ldconfig -r $ROOTDIR

# miscellaneous
mkdir -p sys
mkdir -p mnt
mkdir -p proc
