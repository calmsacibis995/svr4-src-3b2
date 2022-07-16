#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:syssetup/Makefile	1.4"

HELPFILES=Help nodename.Help datetime.Help password.Help setup.Help

HELPDEST=syssetup/Help nodename/Help datetime/Help password/Help setup/Help

O_SHFILES=\
	dateset gettz syspwck syspwend datechk
O_MSGFILES=\
	nextstep
O_DFILES=\
	syssetup.menu \
	datetime.menu nodename.menu \
	Form.datetime Menu.timezones Text.datenok Text.dateok \
	Form.nodename Text.nodenotok Text.nodeokay \
	Form.syspasswd Form.chgpasswd Menu.syspasswd Text.nologins \
	Text.setup Form.setup3 Form.setup4 Text.setup2 \
        Form.addusr Form.addpswd Menu.pgrp Menu.sgrp \
	Text.apswdok Text.apswdnok \
	Form.dateset Text.datedisp Form.nodeset Text.nodedisp \
	Form.setadmps Form.setchgps Text.ausrnok Text.dterr \
	Text.dtok Text.nderr Text.ndok Text.pserr Text.psok \
	Text.setlogerr Text.setpserr Text.setpsok

all: $(O_SHFILES) $(O_MSGFILES)

$(O_MSGFILES):
	grep -v "^#ident" id.$(@) > $(@)

clean:

clobber:
	rm -f $(O_SHFILES) $(O_MSGFILES) 

size strip: all
	:

install: all xinstall

PROTO=Prototype
OBJECTS=$(HELPFILES) $(O_SHFILES) $(O_DFILES) $(O_MSGFILES) 
OBJDEST=$(HELPDEST)

include ../Install
