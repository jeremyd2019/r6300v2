#
# Copyright 2005  Hon Hai Precision Ind. Co. Ltd.
#  All Rights Reserved.
# No portions of this material shall be reproduced in any form without the
# written permission of Hon Hai Precision Ind. Co. Ltd.
#
# All information contained in this document is Hon Hai Precision Ind.
# Co. Ltd. company private, proprietary, and trade secret property and
# are protected by international intellectual property laws and treaties.
#
# $Id$
#

include ../config.mk
include ../config.in

#
# Paths
#
#


# Foxconn Perry added start, for zip and unzip 2013/05/09
SUBDIRS += unix


all: 
	make -f unix/Makefile generic

install:
	make -f unix/Makefile install
	
clean:
	make -f unix/Makefile clean

.PHONY: $(SUBDIRS)
