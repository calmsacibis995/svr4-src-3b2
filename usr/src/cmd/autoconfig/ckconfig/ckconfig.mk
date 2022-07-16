#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)autoconfig:ckconfig/ckconfig.mk	1.7"
ROOT=
DIR=$(ROOT)/sbin
INC= $(ROOT)/usr/include
SYMLINK = :
INS = install
LDFLAGS = 
STRIP= strip
FRC =

all: ckconfig

ckconfig: ckconfig.c 
	$(CC) -I$(INC) $(CFLAGS) -o ckconfig ckconfig.c $(LDFLAGS) $(ROOTLIBS)

install: all
	$(STRIP) ckconfig
	-rm -f $(ROOT)/etc/ckconfig
	$(INS) -f $(DIR) -m 0555 -u root -g root ckconfig
	$(INS) -f $(ROOT)/usr/sbin -m 0555 -u root -g root ckconfig
	-$(SYMLINK) /sbin/ckconfig $(ROOT)/etc/ckconfig

clean:
	rm -f *.o

clobber: clean
	rm -f ckconfig
FRC:
