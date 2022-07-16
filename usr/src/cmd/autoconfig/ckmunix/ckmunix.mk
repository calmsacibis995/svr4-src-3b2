#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)autoconfig:ckmunix/ckmunix.mk	1.9"	
ROOT=
DIR=$(ROOT)/sbin
SYMLINK = :
INS = install
INC= $(ROOT)/usr/include
CFLAGS = -O
LDFLAGS = 
STRIP= strip
FRC =

all: ckmunix

ckmunix: ckmunix.c 
	$(CC) -I$(INC) $(CFLAGS) -o ckmunix ckmunix.c $(LDFLAGS) $(ROOTLIBS)

install: all
	-rm -f $(ROOT)/etc/ckmunix
	$(STRIP) ckmunix
	$(INS) -f $(DIR) -m 0555 -u root -g root ckmunix
	$(INS) -f  $(ROOT)/usr/sbin -m 0555 -u root -g root ckmunix
	-$(SYMLINK) /sbin/ckmunix $(ROOT)/etc/ckmunix

clean:
	rm -f *.o

clobber: clean
	rm -f ckmunix
FRC:
