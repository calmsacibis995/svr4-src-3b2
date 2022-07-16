#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)devinfo:devinfo.mk	1.3.3.1"
ROOT =
INC = $(ROOT)/usr/include
CFLAGS = -O -I$(INC) -Uu3b -Uvax -Updp11 -Uu3b15 -Du3b2
LDFLAGS = -s
MAKE = make "AS=$(AS)" "CC=$(CC)" "LD=$(LD)"
INS = install
FRC =

all:	devinfo

devinfo:
	$(CC) $(CFLAGS) $(LDFLAGS) -o devinfo devinfo.c $(SHLIBS)

install: all
	$(INS) -f $(ROOT)/usr/lbin devinfo

clean:
	-rm -f devinfo.o

clobber: clean
	-rm -f devinfo

FRC:

#
# Header dependencies
#

devinfo: devinfo.c \
	$(INC)/fcntl.h \
	$(INC)/stdio.h \
	$(INC)/sys/id.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/types.h \
	$(INC)/sys/vtoc.h \
	$(INC)/sys/mkdev.h \
	$(FRC)
