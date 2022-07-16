#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cmd-streams:strcmd/strcmd.mk	1.5.2.1"

INC = $(ROOT)/usr/include
INCSYS = $(ROOT)/usr/include
INS = install
USERBIN = $(ROOT)/usr/bin
CFLAGS = -O -s -I$(INCSYS) -I$(INC) -Du3b2
FRC =

PRODUCTS = strchg strconf

.c:
	$(CC) $(CFLAGS) -o $* $*.c $(SHLIBS)

all: $(PRODUCTS)

install: all
	for n in $(PRODUCTS) ; do \
		install -f $(USERBIN) -m 0555 -u root -g root $$n ; \
	done
	
clean:
	

clobber: 
	-rm -f $(PRODUCTS)

FRC:

# 
# Header Dependencies
#
# hidden dependencies:
#	conf.h, evecb.h and types.h are #include'd by stropts.h
#	termios.h is #include'd by termio.h

strchg.o:		strchg.c \
		$(INC)/stdio.h \
		$(INCSYS)/sys/conf.h \
		$(INCSYS)/sys/evecb.h \
		$(INCSYS)/sys/sad.h \
		$(INCSYS)/sys/stat.h \
		$(INCSYS)/sys/stropts.h \
		$(INCSYS)/sys/termio.h \
		$(INCSYS)/sys/termios.h \
		$(INCSYS)/sys/types.h \
		$(FRC)

strconf.o:	strconf.c \
		$(INC)/stdio.h \
		$(INCSYS)/sys/conf.h \
		$(INCSYS)/sys/evecb.h \
		$(INCSYS)/sys/types.h \
		$(INCSYS)/sys/stropts.h \
		$(FRC)
