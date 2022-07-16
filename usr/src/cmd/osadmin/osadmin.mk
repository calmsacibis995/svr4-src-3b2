#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)osadmin:sadmin.mk	1.6"
#	Copyright (c) 1988 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Overall Makefile shipped administrative pieces

ROOT =	/
ADMINHOME =	/usr/admin
BIN =	/usr/bin
LBIN =	/usr/lbin
DIRS =	$(ROOT) $(ROOT)/usr $(ROOT)/install
DEVS =	$(ROOT)/dev/SA/diskette1 $(ROOT)/dev/rSA/diskette1

PARTS =	shell include csub cmain check admin

FILES = README sadmin.mk

all: \
		$(PARTS)
	cd shell;  make
	cd include;  make
	cd csub;  make
	cd cmain;  make
	cd check;  make
	cd admin;  make

install: \
		all $(DIRS) $(DEVS)
	rootdir=`cd $(ROOT); pwd`;  cd shell;  make DIR=$${rootdir}$(LBIN) $@
	rootdir=`cd $(ROOT); pwd`;  cd cmain;  make DIR=$${rootdir}$(LBIN) $@
	rootdir=`cd $(ROOT); pwd`;  cd check;  make DIR=$${rootdir}$(LBIN) $@
	rootdir=`cd $(ROOT); pwd`;  cd admin;  make BIN=$${rootdir}$(BIN) ADMINHOME=$${rootdir}$(ADMINHOME) $@

product productdir:
	@rootdir=`cd $(ROOT); pwd`; \
	cd ../shell;  make DIR=$${rootdir}$(LBIN) $@  && \
	cd ../cmain;  make DIR=$${rootdir}$(LBIN) $@  && \
	cd ../check;  make DIR=$${rootdir}$(LBIN) $@  && \
	cd ../admin;  make BIN=$${rootdir}$(BIN) ADMINHOME=$${rootdir}$(ADMINHOME) $@

clean clobber newmakefile remove:
	rootdir=`cd $(ROOT); pwd`; cd shell;  make DIR=$${rootdir}$(LBIN) $@
	rootdir=`cd $(ROOT); pwd`; cd include;  make DIR=$${rootdir}$(LBIN) $@
	rootdir=`cd $(ROOT); pwd`; cd csub;   make DIR=$${rootdir}$(LBIN) $@
	rootdir=`cd $(ROOT); pwd`; cd cmain;  make DIR=$${rootdir}$(LBIN) $@
	rootdir=`cd $(ROOT); pwd`; cd check;  make DIR=$${rootdir}$(LBIN) $@
	rootdir=`cd $(ROOT); pwd`; cd admin;  make BIN=$${rootdir}$(BIN) ADMINHOME=$${rootdir}$(ADMINHOME) $@

$(DIRS):
	mkdir $@;  chmod 775 $@

$(DEVS):
	@echo The following device needs to be created: $@
	@echo It is created by linking the largest removable disk slice to $@ .

partslist:	$(PARTS)
	@echo $(FILES)  |  tr ' ' '\012'
	@rootdir=`cd $(ROOT); pwd`; \
	cd ../shell;    make DIR=$${rootdir}$(LBIN) $@  |  sed 's;^;shell/;'  && \
	cd ../include;  make DIR=$${rootdir}$(LBIN) $@  |  sed 's;^;include/;'  && \
	cd ../csub;  make DIR=$${rootdir}$(LBIN) $@  |  sed 's;^;csub/;'  && \
	cd ../cmain;  make DIR=$${rootdir}$(LBIN) $@  |  sed 's;^;cmain/;'  && \
	cd ../check;  make DIR=$${rootdir}$(LBIN) $@  |  sed 's;^;check/;'  && \
	cd ../admin;  make BIN=$${rootdir}$(BIN) ADMINHOME=$${rootdir}$(ADMINHOME) $@  |  sed 's;^;admin/;'  && \

srcaudit:
	@ls -d $(PARTS) $(FILES) 2>&1 >/dev/null
	@rootdir=`cd $(ROOT); pwd`; \
	cd ../shell;    make DIR=$${rootdir}$(LBIN) $@  |  sed 's;^;shell/;'  && \
	cd ../include;  make DIR=$${rootdir}$(LBIN) $@  |  sed 's;^;include/;'  && \
	cd ../csub;  make DIR=$${rootdir}$(LBIN) $@  |  sed 's;^;csub/;'  && \
	cd ../cmain;  make DIR=$${rootdir}$(LBIN) $@  |  sed 's;^;cmain/;'  && \
	cd ../check;  make DIR=$${rootdir}$(LBIN) $@  |  sed 's;^;check/;'  && \
	cd ../admin;  make BIN=$${rootdir}$(BIN) ADMINHOME=$${rootdir}$(ADMINHOME) $@  |  sed 's;^;admin/;'  && \

