#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)autoconfig:nodgmon/nodgmon.mk	1.8"	

ROOT=
DIR=$(ROOT)/usr/sbin
SYMLINK = :
INS = install
INC = $(ROOT)/usr/include
CFLAGS = -O
STRIP= strip
LDFLAGS = 
FRC =

all: nodgmon

nodgmon: nodgmon.c 
	$(CC) -I$(INC) $(CFLAGS) -o nodgmon nodgmon.c $(LDFLAGS) $(SHLIBS)

install: all
	-rm -f $(ROOT)/etc/nodgmon
	$(STRIP) nodgmon
	$(INS) -f $(DIR) -m  0555 -u root -g root nodgmon
	-$(SYMLINK) /usr/sbin/nodgmon $(ROOT)/etc/nodgmon

clean:
	rm -f *.o

clobber: clean
	rm -f nodgmon
FRC:
