#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)bkrs:rsintf.d/restores/rsoper/Makefile	1.5"
OAMBASE=$(ROOT)/usr/sadm/sysadm
DIR = $(ROOT)/bin
INC = $(ROOT)/usr/include
RSODIR = $(OAMBASE)/add-ons/sysadm/restores/respond
INSTALL = install
STRIP = strip
SIZE = size
PRODUCT = respond

$(PRODUCT): all

.sh:
	cp $< $*

O_DFILES=Form.rsoper Menu.methods Text.rsoper Text.err Help

all: $(O_SHFILES)

clean:

clobber:

install: all $(RSODIR)
	for i in $(O_DFILES) ;\
	do \
		$(INSTALL) -m 644 -g bin -u bin -f $(RSODIR) $$i ;\
	done

size: all
	$(SIZE) $(MAINS)

strip: all
	$(STRIP) $(MAINS)

$(RSODIR):
	if [ ! -d `dirname $(RSODIR)` ] ;\
	then \
		mkdir `dirname $(RSODIR)` ;\
	fi
	if [ ! -d $(RSODIR) ] ;\
	then \
		mkdir $(RSODIR) ;\
	fi
