#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:machinemgmt/Makefile	1.3"

O_SHFILES=\
	chgstate
O_DFILES=\
	machine.menu \
	Text.boot Text.booterr \
	Form.firm Text.firm \
	Text.flpky1 Text.flpky2 Form.flpky \
	Form.power Text.power \
	Form.reboot Text.reboot \
	Text.whoson

HELPFILES=\
	Help \
	boot.Help firm.Help flpky.Help power.Help reboot.Help whoson.Help

HELPDEST=\
	machinemgmt/Help \
	boot/Help firm/Help flpky/Help power/Help reboot/Help whoson/Help

all: $(O_SHFILES)

clean:

clobber: clean
	rm -f $(O_SHFILES)

size strip: all
	:

install: all xinstall

PROTO=Prototype
OBJECTS=$(HELPFILES) $(O_SHFILES) $(O_DFILES)
OBJDEST=$(HELPDEST)
include ../Install
