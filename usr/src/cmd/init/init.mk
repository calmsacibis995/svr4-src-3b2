#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)init:init.mk	1.8.4.10"
TESTDIR = .
FRC =
INS = install
INSDIR = $(ROOT)/sbin
LDFLAGS = -O -s
SYMLINK = :

all: init

init: init.c $(FRC)
	$(CC) $(LDFLAGS) -o $(TESTDIR)/init init.c $(ROOTLIBS)

test:
	rtest $(TESTDIR)/init

install: all
	-rm -f $(ROOT)/etc/init
	-rm -f $(ROOT)/etc/telinit
	-rm -f $(ROOT)/usr/sbin/init
	$(INS) -f $(INSDIR) -o -m 0555 -u root -g sys $(TESTDIR)/init
	$(INS) -f $(ROOT)/usr/sbin -o -m 0555 -u root -g sys $(TESTDIR)/init
	-$(SYMLINK) /sbin/init $(ROOT)/etc/init
	-$(SYMLINK) /sbin/init $(ROOT)/etc/telinit

clean:


clobber: clean
	-rm -f $(TESTDIR)/init

FRC:
