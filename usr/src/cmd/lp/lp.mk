#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)nlp:lp.mk	1.28"
#
# Top level makefile for the LP Print Service component
#


TOP	=	.

include ./common.mk


ROOTLIBS=-dn
PERFLIBS=-dn
SHLIBS	=
NOSHLIBS=-dn


CMDDIR	=	./cmd
LIBDIR	=	./lib
INCDIR	=	./include
ETCDIR	=	./etc
MODELDIR=	./model
FILTDIR	=	./filter
CRONTABDIR=	./crontab

PLACES	=	$(LIBDIR) \
		$(CMDDIR) \
		$(ETCDIR) \
		$(MODELDIR) \
		$(FILTDIR) \
		$(CRONTABDIR)


DIRS	= \
		$(VAR)/lp \
		$(VAR)/lp/logs \
		$(USRLIBLP) \
		$(USRLIBLP)/bin \
		$(USRLIBLP)/model \
		$(ETC)/lp \
		$(ETC)/lp/classes \
		$(ETC)/lp/forms \
		$(ETC)/lp/interfaces \
		$(ETC)/lp/printers \
		$(ETC)/lp/pwheels \
		$(VARSPOOL)/lp \
		$(VARSPOOL)/lp/admins \
		$(VARSPOOL)/lp/requests \
		$(VARSPOOL)/lp/system \
		$(VARSPOOL)/lp/temp \
		$(VARSPOOL)/lp/fifos \
		$(VARSPOOL)/lp/fifos/private \
		$(VARSPOOL)/lp/fifos/public

#####
#
# The above directories have a different group ownership.
#####
GROUP	=	sys

PRIMODE	=	0771
PUBMODE	=	0773

DEBUG	=	-O


all:		libs \
		cmds \
		etcs \
		models \
		filters \
		crontabs
		
#####
#
# Doing "make install" from the top level will install stripped
# copies of the binaries. Doing "make install" from a lower level
# will install unstripped copies. Also, "make install" doesn't
# depend on "all" at the top level, but it does at lower levels.
# This avoids a redundant pass.
#####
install:	all strip dirs
	for d in $(PLACES); \
	do \
		cd $$d; \
		$(MAKE) install; \
		cd ..; \
	done

#####
#
# Lower level makefiles have "clobber" depend on "clean", but
# doing so here would be redundant.
#####
clean clobber:
	for d in $(PLACES); \
	do \
		cd $$d; \
		$(MAKE) $@; \
		cd ..; \
	done

strip:
	if [ -n "$(STRIP)" ]; \
	then \
		$(MAKE) STRIP=$(STRIP) -f lp.mk realstrip; \
	else \
		$(MAKE) STRIP=strip -f lp.mk realstrip; \
	fi

realstrip:
	for d in $(PLACES); \
	do \
		cd $$d; \
		$(MAKE) STRIP=$(STRIP) strip; \
		cd ..; \
	done

dirs:
	for d in $(DIRS); do if [ ! -d $$d ]; then mkdir $$d; fi; done
	$(CH)chown $(OWNER) $(DIRS)
	$(CH)chgrp $(GROUP) $(DIRS)
	$(CH)chmod $(DMODES) $(DIRS)
	$(CH)chmod $(PRIMODE) $(VARSPOOL)/lp/fifos/private
	$(CH)chmod $(PUBMODE) $(VARSPOOL)/lp/fifos/public
	$(SYMLINK) $(ETC) $(VARSPOOL)/lp/admins/lp
	$(SYMLINK) $(USRLIBLP)/bin $(VARSPOOL)/lp/bin
	$(SYMLINK) $(VAR)/lp/logs $(VARSPOOL)/lp/logs
	$(SYMLINK) $(VAR)/lp/logs $(ETC)/lp/logs
	$(SYMLINK) $(USRLIBLP)/model $(VARSPOOL)/lp/model

libs:
	cd $(LIBDIR); $(MAKE) DEBUG="$(DEBUG)" FUNCDCL="$(FUNCDCL)"

cmds:
	cd $(CMDDIR); $(MAKE) DEBUG="$(DEBUG)"

etcs:
	cd $(ETCDIR); $(MAKE) DEBUG="$(DEBUG)"

models:
	cd $(MODELDIR); $(MAKE) DEBUG="$(DEBUG)"

filters:
	cd $(FILTDIR); $(MAKE) DEBUG="$(DEBUG)"

crontabs:
	cd $(CRONTABDIR); $(MAKE) DEBUG="$(DEBUG)"

lint:
	for d in $(PLACES); \
	do \
		cd $$d; \
		$(MAKE) DEBUG="$(DEBUG)" FUNCDCL="$(FUNCDCL)" LINT="$(LINT)" LINTFLAGS="$(LINTFLAGS)" lint
		cd ..; \
	done

lintsrc:
	cd $(LIBDIR); $(MAKE) DEBUG="$(DEBUG)" FUNCDCL="$(FUNCDCL)" LINT="$(LINT)" LINTFLAGS="$(LINTFLAGS)" lintsrc

lintlib:
	cd $(LIBDIR); $(MAKE) DEBUG="$(DEBUG)" FUNCDCL="$(FUNCDCL)" LINT="$(LINT)" LINTFLAGS="$(LINTFLAGS)" lintlib
