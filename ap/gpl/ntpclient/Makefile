#***************************************************************************
#***
#***    Copyright 2005  Hon Hai Precision Ind. Co. Ltd.
#***    All Rights Reserved.
#***    No portions of this material shall be reproduced in any form without the
#***    written permission of Hon Hai Precision Ind. Co. Ltd.
#***
#***    All information contained in this document is Hon Hai Precision Ind.  
#***    Co. Ltd. company private, proprietary, and trade secret property and 
#***    are protected by international intellectual property laws and treaties.
#***
#****************************************************************************

include ../config.in
include ../config.mk

CFLAGS	+= -I. -Wall
#CFLAGS	+= -g -DDEBUG
CFLAGS	+= -s -O2

STRIPCMD := $(STRIP) --strip-all --remove-section=.note --remove-section=.comment --remove-section=.pdr

TARGET = ntpclient
OBJS = ntpclient.o

$(TARGET): $(OBJS)
	$(CC) -o $@ $^
	$(STRIPCMD) $@ 

$(OBJS): %.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	for i in $(OBJS); do rm -f $$i; done
	for i in $(OBJS:%.o=.%.depend); do rm -f $$i; done
	rm -f $(TARGET)
	
install: $(TARGET)
	install -d $(TARGETDIR)/sbin
	install -m 755 $(TARGET) $(TARGETDIR)/sbin
	$(STRIP) $(TARGETDIR)/sbin/$(TARGET)

%.o: .%.depend
.%.depend: %.c
	$(CC) $(CFLAGS) -M $< > $@

-include $(OBJS:%.o=.%.depend)

