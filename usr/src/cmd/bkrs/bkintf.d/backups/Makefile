#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)bkrs:bkintf.d/backups/Makefile	1.3.1.9"

INC = $(ROOT)/usr/include
BKBIN=$(ROOT)/usr/sadm/bkup/bin
OAMBASE=$(ROOT)/usr/sadm/sysadm
SBIN=$(ROOT)/sbin
USBIN=$(ROOT)/usr/sbin
ETCBKUP=$(ROOT)/etc/bkup
BACKUPDIR = $(OAMBASE)/add-ons/sysadm/backup_service
SADMDIR=$(ROOT)/var/sadm
PKGDIR=$(SADMDIR)/pkg/sysadm
MIDIR = $(PKGDIR)/save/intf_install
MIFILE = bkup.mi
HELPFILE = Help
INSTALL = install
STRIP = strip
SIZE = size
PRODUCTS=backup bkhist bkoper bkrem bksched bksetup bkstat

backups: all

all: $(PRODUCTS) $(MIFILE) $(HELPFILE)

.sh:
	cp $< $*

$(PRODUCTS):
	cd $(@); $(MAKE) -$(MAKEFLAGS) $(@); cd ..

$(MIFILE):
	grep -v "^#ident" 08455426588.mi >$(MIFILE)

clean:

clobber:
	for f in $(PRODUCTS) ;\
	do \
		cd $$f ;\
		$(MAKE) -$(MAKEFLAGS) $(@) ;\
		cd .. ;\
	done
	rm -f $(MIFILE)

install: all $(BACKUPDIR) $(PKGDIR) $(MIDIR)
	for f in $(PRODUCTS) ;\
	do \
		cd $$f ;\
		$(MAKE) -$(MAKEFLAGS) $(@) ;\
		cd .. ;\
	done

	$(INSTALL) -m 644 -g bin -u bin -f $(MIDIR) $(MIFILE)
	$(INSTALL) -m 644 -g bin -u bin -f $(BACKUPDIR) $(HELPFILE)

size: all
	$(SIZE)

strip: all
	$(STRIP)

$(BACKUPDIR):
	if [ ! -d `dirname $(BACKUPDIR)` ] ;\
	then \
		mkdir `dirname $(BACKUPDIR)` ;\
	fi
	if [ ! -d $(BACKUPDIR) ] ;\
	then \
		mkdir $(BACKUPDIR) ;\
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
