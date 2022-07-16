#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)lp:cmd/bsd/Makefile	1.6"

ROOT	=
LPROOT	=	../../..
LPLIB	=	$(LPROOT)/lib
LPINC	=	$(LPROOT)/include
USRBIN	=	$(ROOT)/usr/bin
USRLIB	=	$(ROOT)/usr/lib
INC	=	$(ROOT)/usr/include
INCSYS	=	$(INC)/sys

DEBUG	=

TOOLS = lpc lpq lpr lprm lptest

all install clean clobber strip:
	for d in $(TOOLS); \
	do \
		cd $$d; \
		$(MAKE) LPINC=$(LPINC) LPLIB=$(LPLIB) DEBUG="$(DEBUG)" $@; \
		cd ..; \
	done
