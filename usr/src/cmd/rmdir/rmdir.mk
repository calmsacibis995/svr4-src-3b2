#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)rmdir:rmdir.mk	1.13"
#	rmdir make file
ROOT=
INSDIR = $(ROOT)/usr/bin
INC = $(ROOT)/usr/include
CFLAGS = -O  -I$(INC)
SYMLINK = :
INS = install
LDFLAGS = -s -lgen $(PERFLIBS)

all:	rmdir

rmdir:
	$(CC) $(CFLAGS) -o rmdir rmdir.c  $(LDFLAGS)

install: all
	$(INS) -f $(INSDIR) -m 0555 -u bin -g bin rmdir

clean:
	-rm -f rmdir.o

clobber: clean
	rm -f rmdir
