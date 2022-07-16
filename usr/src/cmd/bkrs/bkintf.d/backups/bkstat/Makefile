#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)bkrs:bkintf.d/backups/bkstat/Makefile	1.1.1.8"

BKBIN=$(ROOT)/usr/sadm/bkup/bin
OAMBASE=$(ROOT)/usr/sadm/sysadm
SBIN=$(ROOT)/sbin
USBIN=$(ROOT)/usr/sbin
ETCBKUP=$(ROOT)/etc/bkup
DIR = $(ROOT)/bin
INC = $(ROOT)/usr/include
BKSTATDIR = $(OAMBASE)/add-ons/sysadm/backup_service/status
INSTALL = install
STRIP = strip
SIZE = size
PRODUCTS=full limit modify select

rsstat: all

.sh:
	cp $< $*

O_HELPFILE = Help

all: $(O_HELPFILE) $(PRODUCTS)

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

install: all $(BKSTATDIR)
	for f in $(PRODUCTS) ; \
	do \
		cd $$f; \
		$(MAKE) -$(MAKEFLAGS) $(@) ; \
		cd ..; \
	done

	$(INSTALL) -m 644 -g bin -u bin -f $(BKSTATDIR) $(O_HELPFILE)

size: all
	$(SIZE)

strip: all
	$(STRIP)

$(BKSTATDIR):
	if [ ! -d `dirname $(BKSTATDIR)` ] ;\
	then \
		mkdir `dirname $(BKSTATDIR)` ;\
	fi
	if [ ! -d $(BKSTATDIR) ] ;\
	then \
		mkdir $(BKSTATDIR) ;\
	fi
