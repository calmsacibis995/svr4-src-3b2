#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)bkrs:bkintf.d/backups/bkhist/Makefile	1.7"

BKBIN=$(ROOT)/usr/sadm/bkup/bin
OAMBASE=$(ROOT)/usr/sadm/sysadm
SBIN=$(ROOT)/sbin
USBIN=$(ROOT)/usr/sbin
ETCBKUP=$(ROOT)/etc/bkup
DIR = $(ROOT)/bin
INC = $(ROOT)/usr/include
BHISTDIR = $(OAMBASE)/add-ons/sysadm/backup_service/history
INSTALL = install
STRIP = strip
SIZE = size
PRODUCTS=full limit select

bkhist: all

O_SHFILES =

O_HELPFILE = Help

all: $(O_SHFILES) $(O_HELPFILE) $(PRODUCTS)

.sh:
	cp $< $*

$(PRODUCTS):
	cd $(@); $(MAKE) -$(MAKEFLAGS) $(@); cd ..

clean:

clobber:
	for f in $(PRODUCTS) ;\
	do \
		cd $$f ;\
		$(MAKE) -$(MAKEFLAGS) $(@) ;\
		cd .. ;\
	done

install: all $(BHISTDIR)
	for f in $(PRODUCTS) ;\
	do \
		cd $$f ;\
		$(MAKE) -$(MAKEFLAGS) $(@) ;\
		cd .. ;\
	done

	$(INSTALL) -m 644 -g bin -u bin -f $(BHISTDIR) $(O_HELPFILE)

size: all
	$(SIZE)

strip: all
	$(STRIP)

$(BHISTDIR):
	if [ ! -d `dirname $(BHISTDIR)` ] ;\
	then \
		mkdir `dirname $(BHISTDIR)` ;\
	fi
	if [ ! -d $(BHISTDIR) ] ;\
	then \
		mkdir $(BHISTDIR) ;\
	fi
