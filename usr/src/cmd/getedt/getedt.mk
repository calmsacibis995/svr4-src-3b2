#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)getedt:getedt.mk	1.4.3.1"

ROOT =
INC = $(ROOT)/usr/include
CFLAGS = -O -I$(INC) -Uu3b -Uvax -Updp11 -Uu3b15 -Du3b2
LDFLAGS = -s
MAKE = make "AS=$(AS)" "CC=$(CC)" "LD=$(LD)"
INS = install
FRC =

all:	 getedt

install: all
	$(INS) -f $(ROOT)/usr/lbin -m 0555 -u root -g sys getedt

getedt:
	$(CC) $(CFLAGS) $(LDFLAGS) -o getedt getedt.c $(SHLIBS)

clean:
	-rm -f getedt.o

clobber: clean
	-rm -f getedt

FRC:

#
# Header dependencies
#

getedt: getedt.c \
	$(INC)/fcntl.h \
	$(INC)/stdio.h \
	$(INC)/sys/hdeioctl.h \
	$(INC)/sys/hdelog.h \
	$(INC)/sys/id.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/types.h \
	$(INC)/sys/vtoc.h \
	../bbh/edio.h \
	../bbh/hdecmds.h \
	$(FRC)
