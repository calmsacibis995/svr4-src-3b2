#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:devices/Makefile	1.5"

DIRS=copy format erase display partition remove

O_SHFILES= \
	getdlst showgrp valdrive getdisk valdisk ckdisk
O_DFILES= \
	Help \
	Text.diskerr Menu.disk Menu.drive devices.menu

all:	$(O_SHFILES)
	@for i in $(DIRS) ;\
	do \
		echo "\tcd $$i && $(MAKE) -f Makefile $@" ;\
		if cd $$i ;\
		then \
			$(MAKE) -f Makefile $@ ;\
			cd .. ;\
		fi ;\
	done

size strip clean:
	@for i in $(DIRS) ;\
	do \
		echo "\tcd $$i && $(MAKE) -f Makefile $@" ;\
		if cd $$i ;\
		then \
			$(MAKE) -f Makefile $@ ;\
			cd .. ;\
		fi ;\
	done

clobber: clean
	rm -f $(O_SHFILES)
	@for i in $(DIRS) ;\
	do \
		echo "\tcd $$i && $(MAKE) -f Makefile $@" ;\
		if cd $$i ;\
		then \
			$(MAKE) -f Makefile $@ ;\
			cd .. ;\
		fi ;\
	done

install: $(O_SHFILES) xinstall 
	@for i in $(DIRS) ;\
	do \
		echo "\tcd $$i && $(MAKE) -f Makefile $@" ;\
		if cd $$i ;\
		then \
			$(MAKE) -f Makefile $@ ;\
			cd .. ;\
		fi ;\
	done

PROTO=Prototype
OBJECTS=$(O_DFILES) $(O_SHFILES)
include ../Install
