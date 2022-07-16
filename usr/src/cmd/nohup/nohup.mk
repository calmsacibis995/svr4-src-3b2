#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)nohup:nohup.mk	1.5"
#	nohup make file

ROOT =
INS = install
INSDIR = $(ROOT)/usr/bin
CFLAGS = -O $(FFLAG)
LDFLAGS = -s
SOURCE = nohup.c
MAKE = make

all:	$(SOURCE)
	$(CC) $(CFLAGS) $(LDFLAGS) -o nohup nohup.c -lm $(PERFLIBS)

install: all
	$(INS) -f $(INSDIR) -m 0555 -u bin -g bin nohup

clean:
	rm -f nohup.o

clobber:	clean
	  rm -f nohup
