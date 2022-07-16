#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)prtconf:prtconf.mk	1.5.3.1"

ROOT =
INC = $(ROOT)/usr/include
CFLAGS = -O -I$(INC)
LDFLAGS = -s
SYMLINK = :
INS = install
FRC =

all:	prtconf

install: all
	-rm -f $(ROOT)/etc/prtconf
	$(INS) -f $(ROOT)/usr/sbin -m 0555 -u root -g sys prtconf
	-$(SYMLINK) /usr/sbin/prtconf $(ROOT)/etc/prtconf

prtconf:
	$(CC) $(CFLAGS) prtconf.c -o prtconf $(LDFLAGS) -lld -lxedt $(SHLIBS)

clean:
	rm -f prtconf.o 

clobber: clean
	rm -f prtconf

#
# Header dependencies
#

prtconf: prtconf.c \
	$(INC)/ctype.h \
	$(INC)/stdio.h \
	$(INC)/sys/edt.h \
	$(INC)/sys/param.h \
	$(INC)/sys/sys3b.h \
	$(INC)/sys/types.h \
	$(INC)/sys/signal.h \
	$(FRC)
