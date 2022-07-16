#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)osadmin3b2:sadmin3b2.mk	1.5"
#	Copyright (c) 1988 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#	Overall Makefile for 3B2 Simple Administration addon pieces.

ROOT =	/
ADMINHOME =	/usr/admin
BIN = /usr/lbin
DIRS =	$(ROOT) $(ROOT)/usr

PARTS =	admin etc

FILES = README sadmin3b2.mk

all: \
		$(PARTS)
	cd admin;  make
	cd etc;  make

install: \
		all $(DIRS)
	rootdir=`cd $(ROOT); pwd`;  cd admin;  make BIN=$${rootdir}$(BIN) ADMINHOME=$${rootdir}$(ADMINHOME) $@
	rootdir=`cd $(ROOT); pwd`;  cd etc;  make ROOT=$${rootdir} $@

product productdir:
	@rootdir=`cd $(ROOT); pwd`; \
	cd admin;  make BIN=$${rootdir}$(BIN) ADMINHOME=$${rootdir}$(ADMINHOME) $@; \
	cd ../etc;  make ROOT=$${rootdir} $@

clean clobber remove:
	rootdir=`cd $(ROOT); pwd`; \
	cd admin;  make BIN=$${rootdir}$(BIN) ADMINHOME=$${rootdir}$(ADMINHOME) $@; \
	cd ../etc;  make ROOT=$${rootdir} $@

$(DIRS):
	mkdir $@;  chmod 775 $@

partslist:	$(PARTS)
	@echo $(FILES)  |  tr ' ' '\012'
	@rootdir=`cd $(ROOT); pwd`; \
	cd admin;  make BIN=$${rootdir}$(BIN) ADMINHOME=$${rootdir}$(ADMINHOME) $@  |  sed 's;^;admin/;'; \
	cd ../etc;  make ROOT=$${rootdir} $@  |  sed 's;^;etc/;'

srcaudit:
	@ls -d $(PARTS) $(FILES) 2>&1 >/dev/null
	@rootdir=`cd $(ROOT); pwd`; \
	cd admin;  make BIN=$${rootdir}$(BIN) ADMINHOME=$${rootdir}$(ADMINHOME) $@  |  sed 's;^;admin/;'; \
	cd ../etc;  make ROOT=$${rootdir} $@  |  sed 's;^;etc/;'
