#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:devices/display/Makefile	1.4"

O_SHFILES=\
	dispdisk
O_DFILES=\
	Help \
	display.menu Form.disk Text.display Text.disperr

all: $(O_SHFILES)

clean:

clobber: clean
	rm -f $(O_SHFILES)

size strip: all
	:

install: all xinstall

PROTO=Prototype
OBJECTS=$(O_SHFILES) $(O_DFILES)
include ../../Install
