#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)vmkfs:vmkfs.mk	1.2.4.2"

ROOT =
INC = $(ROOT)/usr/include
CFLAGS = -O -I$(INC)
LDFLAGS = -s
INS = install
FRC =

all:	 vmkfs

install: all
	$(INS) -f $(ROOT)/usr/lbin -m 0555 -u bin -g bin vmkfs

vmkfs:
	$(CC) $(CFLAGS) $(LDFLAGS) -o vmkfs vmkfs.c $(NOSHLIBS)

clean:
	-rm -f vmkfs.o

clobber: clean
	-rm -f vmkfs

FRC:

#
# Header dependencies
#

vmkfs: vmkfs.c \
	$(INC)/errno.h \
	$(INC)/fcntl.h \
	$(INC)/unistd.h \
	$(INC)/stdlib.h \
	$(INC)/string.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/id.h \
	$(INC)/sys/mkdev.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/types.h \
	$(INC)/sys/vtoc.h \
	$(FRC)
