#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)format:format.mk	1.4"

ROOT =
INC = $(ROOT)/usr/include
CFLAGS = -O -I$(INC)
LDFLAGS = -s
SYMLINK = :
INS = install
FRC =

all:	format

install: all
	-rm -f $(ROOT)/etc/format
	$(INS) -f $(ROOT)/usr/sbin -m 0555 -u root -g root format
	-$(SYMLINK) /usr/sbin/format $(ROOT)/etc/format

format:
	$(CC) $(CFLAGS) $(LDFLAGS) -o format format.c $(SHLIBS)

clean:
	-rm -f format.o

clobber: clean
	-rm -f format

FRC:

#
# Header dependencies
#

format: format.c \
	$(INC)/stdio.h \
	$(INC)/fcntl.h \
	$(INC)/errno.h \
	$(INC)/sys/extbus.h \
	$(FRC)
