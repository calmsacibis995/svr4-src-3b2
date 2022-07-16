#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)fmtflop:fmtflop.mk	1.3.4.1"

ROOT =
INC = $(ROOT)/usr/include
CFLAGS = -O -I$(INC) -Uu3b -Uvax -Updp11 -Uu3b15 -Du3b2
LDFLAGS = -s
SYMLINK = :
MAKE = make "AS=$(AS)" "CC=$(CC)" "LD=$(LD)"
INS = install

all:	fmtflop

install: all
	-rm -f $(ROOT)/etc/fmtflop
	$(INS) -f $(ROOT)/usr/sbin -m 0555 -u bin -g bin fmtflop
	-$(SYMLINK) /usr/sbin/fmtflop $(ROOT)/etc/fmtflop

fmtflop:
	$(CC) $(CFLAGS) $(LDFLAGS) -o fmtflop fmtflop.c $(SHLIBS)

clean:
	-rm -f fmtflop.o

clobber: clean
	-rm -f fmtflop

FRC:

#
# Header dependencies
#

fmtflop: fmtflop.c \
	$(INC)/errno.h \
	$(INC)/fcntl.h \
	$(INC)/stdio.h \
	$(INC)/signal.h \
	$(INC)/sys/diskette.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/if.h \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/lock.h \
	$(INC)/sys/mkdev.h \
	$(INC)/sys/select.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/types.h \
	$(INC)/sys/vtoc.h \
	$(FRC)
