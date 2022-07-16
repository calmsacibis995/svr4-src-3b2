#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:diagnostics/Makefile	1.4"

O_DFILES=\
	diag.menu Text.repair1 Text.repair2 Text.repair3 \
	Form.report Text.report1 Text.report2
HELPFILES=\
	Help \
	repair.Help report.Help

HELPDEST=\
	diagnostics/Help \
	repair/Help report/Help

all:

clean:

clobber: clean

size strip: all
	:

install: all xinstall

PROTO=Prototype
OBJECTS=$(HELPFILES) $(O_DFILES)
OBJDEST=$(HELPDEST)
include ../Install
