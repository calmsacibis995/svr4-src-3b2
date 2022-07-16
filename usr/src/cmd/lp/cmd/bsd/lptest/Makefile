#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)lp:cmd/bsd/lptest/Makefile	1.9"

ROOT	=
RM	=	/bin/rm -f
INS	=	install
STRIP	=	strip
LINT	=	lint

USRUCB	=	$(ROOT)/usr/ucb

OWNER	=	lp
GROUP	=	lp

EMODES	=	0555

INC	=	$(ROOT)/usr/include
INCSYS	=	$(INC)/sys

SHLIBS	=
DEBUG	=	-DDEBUG
XCFLAGS	=	$(DEBUG) -O
CFLAGS	=	-I$(INC) $(XCFLAGS)
LDFLAGS	=	$(SHLIBS)

SRCS	=	lptest.c

OBJS	=	$(SRCS:.c=.o)

CMD	=	lptest

all:	$(CMD)

install:	all strip
	$(INS) -m $(EMODES) -u $(OWNER) -g $(GROUP) -f $(USRUCB) $(CMD)

clean:
	$(RM) $(OBJS) a.out core

clobber:	clean
	$(RM) $(CMD)

strip:
	$(STRIP) $(CMD)

stamp:

lint:
	$(LINT) $(CFLAGS) $(SRCS)

ctags:
	$(TAGS) $(SRCS)
