#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)prtvtoc:prtvtoc.mk	1.6.6.2"
ROOT =
INC = $(ROOT)/usr/include
CFLAGS = -O -I$(INC)
LDFLAGS = -s
INS = install
SYMLINK = :
FRC =

all:	 prtvtoc

install: all
	-rm -f $(ROOT)/etc/prtvtoc
	$(INS) -f $(ROOT)/usr/sbin -m 0555 -u root -g sys prtvtoc
	-$(SYMLINK) /usr/sbin/prtvtoc $(ROOT)/etc/prtvtoc

prtvtoc:
	$(CC) $(CFLAGS) $(LDFLAGS) -o prtvtoc prtvtoc.c $(NOSHLIBS)

clean:
	-rm -f prtvtoc.o

clobber: clean
	-rm -f prtvtoc

FRC:

#
# Header dependencies
#

prtvtoc: prtvtoc.c \
	$(INC)/errno.h \
	$(INC)/fcntl.h \
	$(INC)/unistd.h \
	$(INC)/stdlib.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/types.h \
	$(INC)/sys/vtoc.h \
	$(FRC)
