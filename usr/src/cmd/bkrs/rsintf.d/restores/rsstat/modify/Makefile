#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)bkrs:rsintf.d/restores/rsstat/modify/Makefile	1.4"
OAMBASE=$(ROOT)/usr/sadm/sysadm
DIR = $(ROOT)/bin
INC = $(ROOT)/usr/include
CHGDIR = $(OAMBASE)/add-ons/sysadm/restores/status/modify
INSTALL = install
STRIP = strip
SIZE = size
PRODUCT = modify

$(PRODUCT): all

.sh:
	cp $< $*

O_DFILES=Form.modify Text.modok Text.modnok Help

all: $(O_SHFILES)

clean:

clobber:

install: all $(CHGDIR)
	for i in $(O_DFILES) ;\
	do \
		$(INSTALL) -m 644 -g bin -u bin -f $(CHGDIR) $$i ;\
	done

size: all
	$(SIZE) $(MAINS)

strip: all
	$(STRIP) $(MAINS)

$(CHGDIR):
	if [ ! -d `dirname $(CHGDIR)` ] ;\
	then \
		mkdir `dirname $(CHGDIR)` ;\
	fi
	if [ ! -d $(CHGDIR) ] ;\
	then \
		mkdir $(CHGDIR) ;\
	fi
