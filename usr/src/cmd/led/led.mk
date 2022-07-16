#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)led:led.mk	1.3.3.1"

ROOT =
INC = $(ROOT)/usr/include
CFLAGS = -O -I$(INC) -Uu3b -Uvax -Updp11 -Uu3b15 -Du3b2
LDFLAGS = -s
SYMLINK = :
MAKE = make "AS=$(AS)" "CC=$(CC)" "LD=$(LD)"
INS = install
FRC =

all:	led

led:
	$(CC) $(CFLAGS) $(LDFLAGS) -o led led.c $(ROOTLIBS)

install: all
	-rm -f $(ROOT)/etc/led
	$(INS) -f $(ROOT)/sbin -m 0550 -u bin -g bin led
	$(INS) -f $(ROOT)/usr/sbin -m 0550 -u bin -g bin led
	-$(SYMLINK) /sbin/led $(ROOT)/etc/led

clean:
	-rm -f led.o

clobber: clean
	-rm -f led

FRC:

#
# Header dependencies
#

led: led.c \
	$(INC)/sys/types.h \
	$(INC)/stdio.h \
	$(INC)/sys/sys3b.h \
	$(FRC)
