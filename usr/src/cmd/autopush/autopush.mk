#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)autopush:autopush.mk	1.10"
#
# auopush.mk:
# makefile for autopush(1M) command
#

INS = install
INC = $(ROOT)/usr/include
INCSYS = $(ROOT)/usr/include
SYMLINK = :
OPT = -O
CFLAGS = -I$(INCSYS) -I$(INC) ${OPT}
LDFLAGS = -s
DIR = $(ROOT)/sbin

all:	autopush

install:	all
		-rm -f $(ROOT)/etc/autopush
		-rm -f $(ROOT)/usr/sbin/autopush
		$(INS) -f $(DIR) -m 0555 -u root -g sys autopush
		$(INS) -f $(ROOT)/usr/sbin -m 0555 -u root -g sys autopush
		-$(SYMLINK) /sbin/autopush $(ROOT)/etc/autopush

clean:
	-rm -f *.o

clobber: clean
	-rm -f autopush

autopush:	autopush.c \
		$(INCSYS)/sys/types.h \
		$(INCSYS)/sys/sad.h \
		$(INCSYS)/sys/conf.h \
		$(INC)/stdio.h \
		$(INC)/fcntl.h \
		$(INC)/errno.h \
		$(INC)/ctype.h
		$(CC) $(CFLAGS) -o autopush autopush.c $(LDFLAGS) $(ROOTLIBS)
