#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kill:kill.mk	1.12"

ROOT =

DIR = $(ROOT)/usr/bin

INS = install

SIZE = size

MAINS = kill

ALL:	$(MAINS)

all : ALL

kill:
	echo /bin/sh -c \"kill $$\*\" > kill

clean:
	rm -f $(MAINS)

clobber:
	rm -f $(MAINS)

install: ALL
	$(INS) -f $(DIR) -m 00555 -u bin -g bin kill

newmakefile:

size: 

strip: 
