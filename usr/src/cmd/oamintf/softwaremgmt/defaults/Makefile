#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:softwaremgmt/defaults/Makefile	1.2"

O_SHFILES=
O_DFILES=\
	Form.sd_add Form.sd_list Form.sd_mod Form.sd_remove \
	defaults.menu Menu.logins
HELPFILES=\
	Help
HELPDEST=\
	defaults/Help

all: $(O_SHFILES)

clean:
	:

clobber: clean
	rm -f $(O_SHFILES)

size strip: all
	:

install: all xinstall

PROTO=../Prototype
OBJECTS=$(HELPFILES) $(O_SHFILES) $(O_DFILES)
OBJDEST=$(HELPDEST)
include ../../Install
