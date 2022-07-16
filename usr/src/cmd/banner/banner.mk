#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)banner:banner.mk	1.4"
#	banner make file

ROOT =
INS = install
INSDIR = $(ROOT)/usr/bin
CFLAGS = -O $(FFLAG)
LDFLAGS = -s
SOURCE = banner.c
MAKE = make

all:	$(SOURCE)
	$(CC) $(CFLAGS) $(LDFLAGS) -o banner banner.c -lm $(SHLIBS)

install:	all
	$(INS) -f $(INSDIR) -m 0555 -u bin -g bin banner

clean:
	rm -f banner.o

clobber:	clean
	  rm -f banner
