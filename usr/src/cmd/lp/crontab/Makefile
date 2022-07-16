#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)lp:crontab/Makefile	1.7"

#
# Makefile for lp/crontab
#


INS	=	install

LINT	=	$(PFX)lint

CRONTAB	=	$(ROOT)/var/spool/cron/crontabs

OWNER	=	root
GROUP	=	root
MODE	=	444

SRC	=	lp

all:

install:
	$(INS) -m $(MODE) -u $(OWNER) -g $(GROUP) -f $(CRONTAB) $(SRC)

clobber:

clean:

strip:
