#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)nlp:filter/Makefile	1.14"
#
# Makefile for lp/filter
#



TOP	=	..

include ../common.mk


DEBUG	=	-g

CFLAGS	=	$(DEBUG) -I$(LPINC) -I$(INC)


MISC	= \
		slow.filter


all:		$(MISC)
	cd postscript; $(MAKE)

install:
	for misc in $(MISC); \
	do \
		$(INS) -m $(EMODES) -u $(OWNER) -g $(GROUP) -f $(LPBINDIR) $$misc; \
	done
	cd postscript; $(MAKE) install

clean clobber:
	cd postscript; $(MAKE) $@

strip:

lint:
