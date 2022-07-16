#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)bkrs:bkintf.d/backups/bksched/Makefile	1.7

BKBIN=$(ROOT)/usr/sadm/bkup/bin
OAMBASE=$(ROOT)/usr/sadm/sysadm
SBIN=$(ROOT)/sbin
USBIN=$(ROOT)/usr/sbin
ETCBKUP=$(ROOT)/etc/bkup
DIR = $(ROOT)/bin
INC = $(ROOT)/usr/include
BKSCHEDDIR = $(OAMBASE)/add-ons/sysadm/backup_service/schedule
INSTALL = install
STRIP = strip
SIZE = size
PRODUCTS=add display modify remove

rsstat: all

.sh:
	cp $< $*

O_HELPFILE = Help

all: $(O_SHFILES) $(O_HELPFILE) $(PRODUCTS)

$(PRODUCTS):
	cd $(@); $(MAKE) -$(MAKEFLAGS) $(@); cd ..

clean:

clobber:
	for f in $(PRODUCTS) ; \
	do \
		cd $$f; \
		$(MAKE) -$(MAKEFLAGS) $(@) ; \
		cd ..; \
	done

install: all $(BKSCHEDDIR)
	for f in $(PRODUCTS) ; \
	do \
		cd $$f; \
		$(MAKE) -$(MAKEFLAGS) $(@) ; \
		cd ..; \
	done

	$(INSTALL) -m 644 -g bin -u bin -f $(BKSCHEDDIR) $(O_HELPFILE)

size: all
	$(SIZE)

strip: all
	$(STRIP)

$(BKSCHEDDIR):
	if [ ! -d `dirname $(BKSCHEDDIR)` ] ;\
	then \
		mkdir `dirname $(BKSCHEDDIR)` ;\
	fi
	if [ ! -d $(BKSCHEDDIR) ] ;\
	then \
		mkdir $(BKSCHEDDIR) ;\
	fi
