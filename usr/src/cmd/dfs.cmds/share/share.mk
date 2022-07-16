#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)dfs.cmds:share/share.mk	1.3"
ROOT =
TESTDIR = .
INSDIR = $(ROOT)/usr/sbin
INC = $(ROOT)/usr/include
LDFLAGS = 
INS = install
CFLAGS = -O -s
FRC =

all: share

share: share.c 
	$(CC) -I$(INC) $(CFLAGS) -o $(TESTDIR)/share share.c $(LDFLAGS) $(SHLIBS)

install: all
	$(INS) -f $(INSDIR) -m 0555 $(TESTDIR)/share

clean:
	rm -f share.o

clobber: clean
	rm -f $(TESTDIR)/share
FRC:
