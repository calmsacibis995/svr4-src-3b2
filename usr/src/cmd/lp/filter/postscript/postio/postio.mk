#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)lp:filter/postscript/postio/postio.mk	1.3"
#
# makefile for the RS-232 serial interface program for PostScript printers.
#

MAKEFILE=postio.mk
ARGS=all

#
# Common source and header files have been moved to $(COMMONDIR).
#

COMMONDIR=../common

#
# postio doesn't use floating point arithmetic, so the -f flag isn't needed.
#

CFLAGS=-O -DSYSV -I$(COMMONDIR)
SYSTEM=SYSV

CFILES=postio.c ifdef.c slowsend.c

HFILES=postio.h\
       ifdef.h\
       $(COMMONDIR)/gen.h

POSTIO=postio.o ifdef.o slowsend.o

ALLFILES=README $(MAKEFILE) $(HFILES) $(CFILES)


all : postio

install : postio
	@if [ ! -d "$(BINDIR)" ]; then \
	    mkdir $(BINDIR); \
	    chmod 775 $(BINDIR); \
	    chgrp $(GROUP) $(BINDIR); \
	    chown $(OWNER) $(BINDIR); \
	fi
	install -m 775 -u $(OWNER) -g $(GROUP) -f $(BINDIR) postio
#	cp postio $(BINDIR)
#	chmod 775 $(BINDIR)/postio
#	chgrp $(GROUP) $(BINDIR)/postio
#	chown $(OWNER) $(BINDIR)/postio

postio : $(POSTIO)
	@if [ "$(SYSTEM)" = "SYSV" -a -d "$(DKHOSTDIR)" ]; then \
	    EXTRA="-Wl,-L$(DKHOSTDIR)/lib -ldk"; \
	fi; \
	if [ "$(SYSTEM)" = "V9" ]; then \
	    EXTRA="-lipc"; \
	fi; \
	echo "	$(CC) $(CFLAGS) -o postio $(POSTIO) $$EXTRA"; \
	$(CC) $(CFLAGS) -o postio $(POSTIO) $$EXTRA

postio.o : $(HFILES)
slowsend.o : postio.h $(COMMONDIR)/gen.h
ifdef.o : ifdef.h $(COMMONDIR)/gen.h

clean :
	rm -f $(POSTIO)

clobber : clean
	rm -f postio

list :
	pr -n $(ALLFILES) | $(LIST)

