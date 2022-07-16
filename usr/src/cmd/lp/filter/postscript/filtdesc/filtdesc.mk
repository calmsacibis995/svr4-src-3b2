#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)lp:filter/postscript/filtdesc/filtdesc.mk	1.3"
#
# Makefile for lp/filter/postscript/filtdesc
#


RM	=	/bin/rm -f
INS	=	install
CP	=	cp

ETC	=	$(ROOT)/etc
ETCLP	=	$(ROOT)/etc/lp

OWNER	=	lp
GROUP	=	lp
RMODES	=	0444


all:		filter.table

install:	all
	$(INS) -m $(RMODES) -u $(OWNER) -g $(GROUP) -f $(ETCLP) filter.table.i
	$(INS) -m $(RMODES) -u $(OWNER) -g $(GROUP) -f $(ETCLP) filter.table

clean:
	$(RM) filter.table

clobber:	clean

strip:

filter.table:
	$(CP) filter.table.i filter.table
