#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)factor:factor.mk	1.9"
#	factor make file

ROOT =
INS = install
INC = $(ROOT)/usr/include
INSDIR = $(ROOT)/usr/bin
CFLAGS = -O $(FFLAG) -I$(INC)
LDFLAGS = -s
SOURCE = factor.c
MAKE = make

all:	$(SOURCE)
	$(CC) $(CFLAGS) $(LDFLAGS) -o factor factor.c -lm $(SHLIBS)

install:	all
	$(INS) -f $(INSDIR) -m 0555 -u bin -g bin factor

clean:
	rm -f factor.o

clobber:	clean
	  rm -f factor
