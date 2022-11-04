#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)npack:cmd/pckd.mk	1.4"
ROOT =
TESTDIR = .
INSDIR = $(ROOT)/usr/bin
INC = $(ROOT)/usr/include
INS = :
CFLAGS = -O -s
FRC =

all:	pckd

pckd:	pckd.c\
	$(INC)/sys/stropts.h
	$(CC) -I$(INC) $(CFLAGS) -o $(TESTDIR)/pckd pckd.c

install: all
	install -n $(INSDIR) $(TESTDIR)/pckd
	cp ./npack $(ROOT)/etc/init.d/npack
	$(CH)chmod 744 $(ROOT)/etc/init.d/npack
	$(CH)chown adm $(ROOT)/etc/init.d/npack
	$(CH)chgrp sys $(ROOT)/etc/init.d/npack

clean:
	rm -f pckd.o

clobber: clean
	rm -f $(TESTDIR)/pckd
FRC:
