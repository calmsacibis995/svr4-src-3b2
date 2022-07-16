#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)getmajor:getmajor.mk	1.3.3.2"

ROOT =
INC = $(ROOT)/usr/include
SYMLINK = :
CFLAGS = -O -I$(INC)
LDFLAGS = -s
INS = install
FRC =

all:	getmajor 

install: all
	-rm -f $(ROOT)/etc/getmajor
	$(INS) -f $(ROOT)/usr/sbin -m 0555 -u bin -g bin getmajor
	-$(SYMLINK) /usr/sbin/getmajor $(ROOT)/etc/getmajor

getmajor:
	$(CC) $(CFLAGS) getmajor.c  -o getmajor $(LDFLAGS) -lxedt $(NOSHLIBS)

clean:
	rm -f getmajor.o

clobber: clean
	rm -f getmajor

#
# Header dependencies
#

getmajor: getmajor.c \
	$(INC)/ctype.h \
	$(INC)/stdio.h \
	$(INC)/sys/edt.h \
	$(INC)/sys/libxedt.h \
	$(INC)/sys/param.h \
	$(INC)/sys/sys3b.h \
	$(INC)/sys/types.h \
	$(FRC)
