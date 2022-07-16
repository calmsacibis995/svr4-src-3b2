#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)fmthard:fmthard.mk	1.4.3.2"

ROOT =
INC = $(ROOT)/usr/include
CFLAGS = -O -I$(INC)
SYMLINK = :
LDFLAGS = -s
INS = install
FRC =

all:	fmthard

install: all
	-rm -f $(ROOT)/etc/fmthard
	-rm -f $(ROOT)/usr/sbin/fmthard
	$(INS) -f $(ROOT)/sbin -m 0555 -u root -g root fmthard
	$(INS) -f $(ROOT)/usr/sbin -m 0555 -u root -g root fmthard
	-$(SYMLINK) /sbin/fmthard $(ROOT)/etc/fmthard

fmthard:
	$(CC) $(CFLAGS) $(LDFLAGS) -o fmthard fmthard.c $(ROOTLIBS)

clean:
	-rm -f fmthard.o

clobber: clean
	-rm -f fmthard

FRC:

#
# Header dependencies
#

fmthard: fmthard.c \
	$(INC)/fcntl.h \
	$(INC)/stdio.h \
	$(INC)/sys/id.h \
	$(INC)/sys/open.h \
	$(INC)/sys/types.h \
	$(INC)/sys/vtoc.h \
	$(FRC)
