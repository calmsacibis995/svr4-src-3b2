#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:devices/erase/Makefile	1.3"

O_SHFILES=\
	floperase
O_DFILES=\
	Help \
	erase.menu Form.diskette \
	Text.erase2 Text.diskette Text.eraserr

all: $(O_SHFILES)

clean:

clobber: clean
	rm -f $(O_SHFILES)

strip size: all
	:

install: all xinstall

PROTO=Prototype
OBJECTS=$(O_SHFILES) $(O_DFILES)
include ../../Install
