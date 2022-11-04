#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)boot:boot/boot/boot.mk	1.7"

ROOT =
INCSYS = $(ROOT)/usr/include
UINC = $(ROOT)/usr/include
LIB = $(ROOT)/lib
DASHO = -O
CFLAGS = $(DASHO) -I$(INCSYS) -I.
SYMLINK = :
STRIP = strip
INS = install
IFILE = lbld
MAPFILE = boot.map
LIBFM = libfm.a
FRC =

FILES = \
	misc.o \
	boot.o

all: boot

boot: $(FILES) $(IFILE) $(LIBFM) $(MAPFILE)
	$(FRC)
	if [ x$(CCSTYPE) = xCOFF ] ; \
	then $(LD) $(LDFLAG) $(IFILE) $(FILES) $(LIBFM) -L$(LIB) -lc -o boot ; \
	else $(LD) $(LDFLAG) -M $(MAPFILE) $(FILES) $(LIBFM) -L$(CCSLIB) -dn -lc -o boot ; \
	fi

install: boot
	-rm -f $(ROOT)/lib/boot
	$(INS) -f $(ROOT)/usr/lib boot
	-$(SYMLINK) $(ROOT)/usr/lib/boot $(ROOT)/lib/boot

$(LIBFM):
	$(MAKE) -f libfm.mk "ROOT=$(ROOT)" "CC=$(CC)" "AS=$(AS)" "AR=$(AR)"

clean:
	-rm -f *.o

clobber: clean
	-rm -f boot
	-rm -f $(LIBFM)

#
# Header dependencies
#

boot.o: boot.c \
	$(INCSYS)/sys/boot.h \
	$(INCSYS)/sys/csr.h \
	$(INCSYS)/sys/elog.h \
	$(INCSYS)/sys/firmware.h \
	$(INCSYS)/sys/id.h \
	$(INCSYS)/sys/immu.h \
	$(INCSYS)/sys/nvram.h \
	$(INCSYS)/sys/sbd.h \
	$(INCSYS)/sys/types.h \
	$(INCSYS)/sys/dma.h \
	$(INCSYS)/sys/psw.h \
	$(INCSYS)/sys/fs/bfs.h \
	$(INCSYS)/sys/fsiboot.h \
	$(FRC)
