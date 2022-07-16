#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)bkrs:rsintf.d/restores/rsstat/Makefile	1.9"

OAMBASE=$(ROOT)/usr/sadm/sysadm
DIR = $(ROOT)/bin
INC = $(ROOT)/usr/include
RSSTATDIR = $(OAMBASE)/add-ons/sysadm/restores/status
INSTALL = install
STRIP = strip
SIZE = size
PRODUCTS=full modify select

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

install: all $(RSSTATDIR)
	for f in $(PRODUCTS) ; \
	do \
		cd $$f; \
		$(MAKE) -$(MAKEFLAGS) $(@) ; \
		cd ..; \
	done

	$(INSTALL) -m 644 -g bin -u bin -f $(RSSTATDIR) $(O_HELPFILE)

size: all
	$(SIZE)

strip: all
	$(STRIP)

$(RSSTATDIR):
	if [ ! -d `dirname $(RSSTATDIR)` ] ;\
	then \
		mkdir `dirname $(RSSTATDIR)` ;\
	fi
	if [ ! -d $(RSSTATDIR) ] ;\
	then \
		mkdir $(RSSTATDIR) ;\
	fi
