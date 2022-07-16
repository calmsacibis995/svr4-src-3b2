#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)errdump:errdump.mk	1.4.3.1"

ROOT =
INC = $(ROOT)/usr/include
CFLAGS = -O -I$(INC) -Uu3b -Uvax -Updp11 -Uu3b15 -Du3b2
LDFLAGS = -s
SYMLINK = :
MAKE = make "AS=$(AS)" "CC=$(CC)" "LD=$(LD)"
INS = install

all:	errdump

errdump:
	$(CC) $(CFLAGS) $(LDFLAGS) -o errdump errdump.c $(SHLIBS)

install: all
	-rm -f $(ROOT)/etc/errdump
	$(INS) -f $(ROOT)/usr/sbin -m 0755 -u bin -g bin errdump
	-$(SYMLINK) /usr/sbin/errdump $(ROOT)/etc/errdump

clean:
	-rm -f errdump.o

clobber: clean
	-rm -f errdump

FRC:

#
# Header dependencies
#

errdump: errdump.c \
	$(INC)/stdio.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/firmware.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/nvram.h \
	$(INC)/sys/nvram.h \
	$(INC)/sys/param.h \
	$(INC)/sys/sys3b.h \
	$(INC)/sys/types.h \
	$(INC)/time.h \
	$(FRC)
