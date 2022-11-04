#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)boot:boot/mboot/mboot.mk	11.4"

ROOT =
LIB = $(ROOT)/lib
INCSYS = $(ROOT)/usr/include
INCLOC = ..
DASHO = -O
CFLAGS = $(DASHO) -I$(INCLOC) -I$(INCSYS)
SIZE = size
DIS = dis
NM = nm
STRIP = strip
CONV = conv
INS = install
SYMLINK = :

LDFLAG =
FRC =

all: mboot

install: mboot
	$(STRIP) mboot
	-rm -f $(ROOT)/lib/mboot
	$(INS) -f $(ROOT)/usr/lib mboot
	-$(SYMLINK) $(ROOT)/usr/lib/mboot $(ROOT)/lib/mboot

mboot: mboot.map\
	mbld \
	mboot.o\
	$(FRC)
	if [ x$(CCSTYPE) = xCOFF ] ; \
	then \
		$(LD) $(LDFLAG) mbld mboot.o -o mboot -L$(LIB) -lc ; \
	else \
		$(LD) $(LDFLAG) -Mmboot.map mboot.o -o mboot -L$(CCSLIB) -dn -lc ; \
	fi

clean:
	rm -f mboot *.o

clobber: clean

debug: mboot
	$(SIZE) mboot > mboot.size
	$(DIS) -L mboot > mboot.dis
	$(NM) -nef mboot > mboot.name
	$(STRIP) mboot
	pr -n mboot.c mbld mboot.dis mboot.name mboot.size | opr -f hole -txr -p land

FRC:

#
# Header dependencies
#

mboot.o: mboot.c \
	$(INCSYS)/sys/boot.h \
	$(INCSYS)/sys/firmware.h \
	$(INCLOC)/sys/param.h \
	$(INCSYS)/sys/types.h \
	$(INCSYS)/sys/vtoc.h \
	$(FRC)
