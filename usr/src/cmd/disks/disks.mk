#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)disks:disks.mk	1.15"

ROOT =
INC = $(ROOT)/usr/include
SYMLINK = :
CFLAGS = -O -I$(INC) -Uu3b -Uvax -Updp11 -Uu3b15 -Du3b2
LDFLAGS = -s
INS = install
FRC =

all:	 disks

install: all
	-rm -f $(ROOT)/etc/disks
	-rm -f $(ROOT)/usr/sbin/disks
	$(INS) -f $(ROOT)/sbin -m 755 -u root -g sys disks
	$(INS) -f $(ROOT)/usr/sbin -m 755 -u root -g sys disks
	$(SYMLINK) /sbin/disks $(ROOT)/etc/disks

disks:
	$(CC) $(CFLAGS) $(LDFLAGS) -o disks disks.c $(ROOTLIBS)

clean:
	-rm -f disks.o

clobber: clean
	-rm -f disks

FRC:

#
# Header dependencies
#

disks: disks.c \
	$(INC)/sys/edt.h \
	$(INC)/sys/mkdev.h \
	$(INC)/unistd.h \
	$(INC)/signal.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/stat.h \
	$(INC)/stdlib.h \
	$(INC)/string.h \
	$(INC)/sys/sys3b.h \
	$(INC)/sys/types.h \
	$(FRC)
