#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)bkrs:bkintf.d/backups/bkrem/display/Makefile	1.3"

BKBIN=$(ROOT)/usr/sadm/bkup/bin
OAMBASE=$(ROOT)/usr/sadm/sysadm
SBIN=$(ROOT)/sbin
USBIN=$(ROOT)/usr/sbin
ETCBKUP=$(ROOT)/etc/bkup
DIR = $(ROOT)/bin
INC = $(ROOT)/usr/include
DISPDIR = $(OAMBASE)/add-ons/sysadm/backup_service/reminder/display
INSTALL = install
STRIP = strip
SIZE = size
PRODUCT = display

$(PRODUCT): all

O_DFILES=Text.display Help

.sh:
	cp $< $*

all: $(O_SHFILES)

clean:

clobber:

install: all $(DISPDIR)
	for i in $(O_DFILES) ;\
	do \
		$(INSTALL) -m 644 -g bin -u bin -f $(DISPDIR) $$i ;\
	done

size: all
	$(SIZE) $(MAINS)

strip: all
	$(STRIP) $(MAINS)

$(DISPDIR):
	if [ ! -d `dirname $(DISPDIR)` ] ;\
	then \
		mkdir `dirname $(DISPDIR)` ;\
	fi
	if [ ! -d $(DISPDIR) ] ;\
	then \
		mkdir $(DISPDIR) ;\
	fi
