#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)dirname:dirname.mk	1.2"

ROOT =
SYMLINK = :
INS = install

all:	install

install:
	cp dirname.sh  dirname
	$(INS) -f $(ROOT)/usr/bin -m 0555 -u bin -g bin dirname

clean:

clobber:	clean
	-rm -rf dirname
