#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)bkrs:rsintf.d/restores/Makefile	1.1.1.8"

OAMBASE=$(ROOT)/usr/sadm/sysadm
DIR = $(ROOT)/bin
INC = $(ROOT)/usr/include
RESTOREDIR = $(OAMBASE)/add-ons/sysadm/restores
SADMDIR=$(ROOT)/var/sadm
PKGDIR=$(SADMDIR)/pkg/sysadm
MIDIR = $(PKGDIR)/save/intf_install
MIFILE = rest.mi
HELPFILE = Help
INSTALL = install
STRIP = strip
SIZE = size
PRODUCTS=restore rsstat setup rsoper

restores: all

all: $(PRODUCTS) $(MIFILE) $(HELPFILE)

.sh:
	cp $< $*

$(PRODUCTS):
	cd $(@); $(MAKE) -$(MAKEFLAGS) $(@); cd ..

$(MIFILE):
	grep -v "^#ident" 16122026388.mi >$(MIFILE)

clean:

clobber:
	for f in $(PRODUCTS) ;\
	do \
		cd $$f ;\
		$(MAKE) -$(MAKEFLAGS) $(@) ;\
		cd .. ;\
	done
	rm -f $(MIFILE)

install: all $(RESTOREDIR) $(PKGDIR) $(MIDIR)
	for f in $(PRODUCTS) ;\
	do \
		cd $$f ;\
		$(MAKE) -$(MAKEFLAGS) $(@) ;\
		cd .. ;\
	done

	$(INSTALL) -m 644 -g bin -u bin -f $(MIDIR) $(MIFILE)
	$(INSTALL) -m 644 -g bin -u bin -f $(RESTOREDIR) $(HELPFILE)

size: all
	$(SIZE)

strip: all
	$(STRIP)

$(RESTOREDIR):
	if [ ! -d `dirname $(RESTOREDIR)` ] ;\
	then \
		mkdir `dirname $(RESTOREDIR)` ;\
	fi
	if [ ! -d $(RESTOREDIR) ] ;\
	then \
		mkdir $(RESTOREDIR) ;\
	fi
$(PKGDIR):
	if [ ! -d `dirname $(PKGDIR)` ] ;\
	then \
		mkdir `dirname $(PKGDIR)` ;\
	fi
	if [ ! -d $(PKGDIR) ] ;\
	then \
		mkdir $(PKGDIR) ;\
	fi
$(MIDIR):
	if [ ! -d `dirname $(MIDIR)` ] ;\
	then \
		mkdir `dirname $(MIDIR)` ;\
	fi
	if [ ! -d $(MIDIR) ] ;\
	then \
		mkdir $(MIDIR) ;\
	fi
