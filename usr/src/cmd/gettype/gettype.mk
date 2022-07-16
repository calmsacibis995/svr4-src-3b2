#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)gettype:gettype.mk	1.4"

ROOT =
INC = $(ROOT)/usr/include
CFLAGS = -O -I$(INC)
LDFLAGS = -s
SYMLINK = :
INS = install
FRC =

all:	gettype

install: all
	-rm -f $(ROOT)/etc/gettype
	$(INS) -f $(ROOT)/usr/sbin -m 0555 -u root -g root gettype
	-$(SYMLINK) /usr/sbin/gettype $(ROOT)/etc/gettype

gettype:
	$(CC) $(CFLAGS) $(LDFLAGS) -o gettype gettype.c $(SHLIBS)

clean:
	-rm -f gettype.o

clobber: clean
	-rm -f gettype

FRC:

#
# Header dependencies
#

gettype: gettype.c \
	$(INC)/stdio.h \
	$(INC)/errno.h \
	$(INC)/sys/pump.h \
	$(INC)/sys/extbus.h \
	$(FRC)
