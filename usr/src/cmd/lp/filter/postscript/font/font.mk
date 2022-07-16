#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)lp:filter/postscript/font/font.mk	1.3"
#ident	"@(#)lp:filter/postscript/font/font.mk	1.2"

#
# makefile for PostScript font files.
#

MAKEFILE=font.mk
ARGS=all

#
# Common header and source files have been moved to $(COMMONDIR).
#

COMMONDIR=../common

#
# makedev is the only program that's compiled. NEEDS $(COMMONDIR) for dev.h.
#

CFLAGS=-O -I$(COMMONDIR)

#
# If you want the %%DocumentFonts: comment from dpost set DOCUMENTFONTS to ON.
# It controls whether the *.name files are left in $(FONTDIR)/devpost.
#

DOCUMENTFONTS=ON

#
# ASCII includes the DESC file and all the ASCII font files. Only the binary
# files, built by makedev, are left in $(FONTDIR)/devpost after we install things.
#

ASCII=DESC ? ??
EXTRA=LINKFILE *.big *.small


all : devpost

install : devpost
	@if [ ! -d $(FONTDIR) ]; then \
	    mkdir $(FONTDIR); \
	    $(CH)chmod 775 $(FONTDIR); \
	    $(CH)chgrp $(GROUP) $(FONTDIR); \
	    $(CH)chown $(OWNER) $(FONTDIR); \
	fi
	@if [ ! -d $(FONTDIR)/devpost ]; then \
	    mkdir $(FONTDIR)/devpost; \
	    $(CH)chmod 775 $(FONTDIR)/devpost; \
	    $(CH)chgrp $(GROUP) $(FONTDIR)/devpost; \
	    $(CH)chown $(OWNER) $(FONTDIR)/devpost; \
	fi
	@if [ ! -d $(FONTDIR)/devpost/charlib ]; then \
	    mkdir $(FONTDIR)/devpost/charlib; \
	    $(CH)chmod 775 $(FONTDIR)/devpost/charlib; \
	    $(CH)chgrp $(GROUP) $(FONTDIR)/devpost/charlib; \
	    $(CH)chown $(OWNER) $(FONTDIR)/devpost/charlib; \
	fi
	if [ "$(DOCUMENTFONTS)" = "ON" ]; then \
	    find devpost/*.name -print \
	    | xargs -i install -m 444 -u $(OWNER) -g $(GROUP) -f $(FONTDIR)/devpost {}; \
	fi
	find devpost/charlib/* -print \
	| xargs -i install -m 444 -u $(OWNER) -g $(GROUP) -f $(FONTDIR)/devpost/charlib {}
	find devpost/*.out -print \
	| xargs -i install -m 444 -u $(OWNER) -g $(GROUP) -f $(FONTDIR)/devpost {}
#	find devpost -print | cpio -pdu $(FONTDIR)
#	cd $(FONTDIR)/devpost; find . -depth -print | xargs chmod ugo+r
#	cd $(FONTDIR)/devpost; find . -depth -print | xargs chgrp $(GROUP)
#	cd $(FONTDIR)/devpost; find . -depth -print | xargs chown $(OWNER)
#	cd $(FONTDIR)/devpost; rm -f $(ASCII) $(EXTRA)
#	if [ "$(DOCUMENTFONTS)" != "ON" ]; then \
#	    cd $(FONTDIR)/devpost; rm -f ?.name ??.name; \
#	fi

devpost ::
	@$(MAKE) -f $(MAKEFILE) makedev
	cd devpost; ../makedev $(ASCII); sh LINKFILE

makedev : $(COMMONDIR)/dev.h makedev.c
	$(CC) $(CFLAGS) -o makedev makedev.c


clean :
	rm -f *.o
	cd devpost; rm -f *.out

clobber : clean
	rm -f makedev

list :
	cd devpost; pr -n ../README ../$(MAKEFILE) $(ASCII) | $(LIST)

