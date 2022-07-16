#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)dfs.cmds:unshareall/unshareall.mk	1.3"
ROOT =
TESTDIR = .
INSDIR = $(ROOT)/usr/sbin/
INC = $(ROOT)/usr/include
INS = install
FRC =

all: unshareall

unshareall: unshareall.sh 
	cp unshareall.sh $(TESTDIR)/unshareall
	chmod 555 $(TESTDIR)/unshareall

install: all
	$(INS) -f $(INSDIR) -m 0555 -u bin -g bin $(TESTDIR)/unshareall

clean:

clobber: clean
	rm -f $(TESTDIR)/unshareall
FRC:
