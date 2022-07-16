#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)bkrs:bkintf.d/backups/bksetup/summary/Makefile	1.3"

BKBIN=$(ROOT)/usr/sadm/bkup/bin
OAMBASE=$(ROOT)/usr/sadm/sysadm
SBIN=$(ROOT)/sbin
USBIN=$(ROOT)/usr/sbin
ETCBKUP=$(ROOT)/etc/bkup
DIR = $(ROOT)/bin
INC = $(ROOT)/usr/include
SUMDIR = $(OAMBASE)/add-ons/sysadm/backup_service/setup/summary
INSTALL = install
STRIP = strip
SIZE = size
PRODUCT = summary

$(PRODUCT): all

.sh:
	cp $< $*

O_DFILES=Form.summary Text.summary Text.errs Help

all: $(O_SHFILES)

clean:

clobber:

install: all $(SUMDIR)
	for i in $(O_DFILES) ;\
	do \
		$(INSTALL) -m 644 -g bin -u bin -f $(SUMDIR) $$i ;\
	done

size: all
	$(SIZE) $(MAINS)

strip: all
	$(STRIP) $(MAINS)

$(SUMDIR):
	if [ ! -d `dirname $(SUMDIR)` ] ;\
	then \
		mkdir `dirname $(SUMDIR)` ;\
	fi
	if [ ! -d $(SUMDIR) ] ;\
	then \
		mkdir $(SUMDIR) ;\
	fi
