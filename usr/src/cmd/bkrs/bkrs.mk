#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)bkrs:bkrs.mk	1.1.2.10"

INC=$(ROOT)/usr/include
LOCAL=hdrs
CC=$(PFX)cc
CFLAGS=
MAKE=make

BKLIB=bklib
RSLIB=rslib
LIBBR=libbrmeth
IOLIB=libadmIO
PRODUCTS=backup bkdaemon bkreg meth restore rsoper bkoper rsstatus bkintf \
	intftools rsintf bkstatus bkhistory rsnotify bkexcept

all: $(PRODUCTS)

tmeth $(IOLIB) $(LIBBR) $(BKLIB) $(RSLIB) $(PRODUCTS): 
	cd $(@).d; $(MAKE) -$(MAKEFLAGS) all; cd ..

install: all
	-[ ! -d $(ROOT)/etc/bkup ] && mkdir $(ROOT)/etc/bkup
	-[ ! -d $(ROOT)/etc/bkup/method ] && mkdir $(ROOT)/etc/bkup/method
	-[ ! -d $(ROOT)/sbin ] && mkdir $(ROOT)/sbin
	-[ ! -d $(ROOT)/usr/lib/getdate ] && mkdir $(ROOT)/usr/lib/getdate
	-[ ! -d $(ROOT)/usr/sbin ] && mkdir $(ROOT)/usr/sbin
	-[ ! -d $(ROOT)/usr/sadm ] && mkdir $(ROOT)/usr/sadm
	-[ ! -d $(ROOT)/usr/sadm/bkup ] && mkdir $(ROOT)/usr/sadm/bkup
	-[ ! -d $(ROOT)/usr/sadm/bkup/bin ] && mkdir $(ROOT)/usr/sadm/bkup/bin
	-[ ! -d $(ROOT)/usr/sadm/sysadm ] && mkdir $(ROOT)/usr/sadm/sysadm
	-[ ! -d $(ROOT)/usr/sadm/sysadm/add-ons ] && mkdir $(ROOT)/usr/sadm/sysadm/add-ons
	-[ ! -d $(ROOT)/usr/sadm/sysadm/add-ons/sysadm ] && mkdir $(ROOT)/usr/sadm/sysadm/add-ons/sysadm
	-[ ! -d $(ROOT)/var ] && mkdir $(ROOT)/var
	-[ ! -d $(ROOT)/var/adm ] && mkdir $(ROOT)/var/adm
	-[ ! -d $(ROOT)/var/adm/bkup ] && mkdir $(ROOT)/var/adm/bkup
	-[ ! -d $(ROOT)/var/adm/bkup/logs ] && mkdir $(ROOT)/var/adm/bkup/logs
	-[ ! -d $(ROOT)/var/adm/bkup/toc ] && mkdir $(ROOT)/var/adm/bkup/toc
	-[ ! -d $(ROOT)/usr/lib/libadmIO ] && mkdir $(ROOT)/usr/lib/libadmIO 
	for f in $(PRODUCTS) $(BKLIB) $(LIBBR) $(RSLIB) $(IOLIB) ; \
	do \
		cd $$f.d; \
		$(MAKE) -$(MAKEFLAGS) $(@) ; \
		cd ..; \
	done

strip lintit clobber clean touch: 
	for f in $(IOLIB) $(BKLIB) $(RSLIB) $(LIBBR) $(PRODUCTS) ; \
	do \
		cd $$f.d; \
		$(MAKE) -$(MAKEFLAGS) $(@) ; \
		cd ..; \
	done

tmeth $(PRODUCTS): $(IOLIB) $(BKLIB) $(LIBBR)
restore rsoper: $(RSLIB)
