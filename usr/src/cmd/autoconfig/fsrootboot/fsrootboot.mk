#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)autoconfig:fsrootboot/fsrootboot.mk	1.3"

ROOT=
DIR=$(ROOT)/sbin
SYMLINK = :
INC = $(ROOT)/usr/include
CFLAGS = -O
LDFLAGS = 
STRIP= strip
FRC =

all: fsrootboot

fsrootboot: fsrootboot.c 
	$(CC) -I$(INC) $(CFLAGS) -o fsrootboot fsrootboot.c $(LDFLAGS) $(ROOTLIBS)

install: all
	$(STRIP) fsrootboot
	$(INS) -f $(DIR) -m 0555 -u root -g root fsrootboot
	$(INS) -f  $(ROOT)/usr/sbin -m 0555 -u root -g root fsrootboot

clean:
	rm -f *.o

clobber: clean
	rm -f fsrootboot
FRC:
