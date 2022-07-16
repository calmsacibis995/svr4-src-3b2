#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)boot:boot/boot/boot.mk	1.10"

ROOT =
INC = $(ROOT)/usr/include
UINC = $(ROOT)/usr/include
DASHO = -O
CFLAGS = $(DASHO) -I$(INC) -I.
SYMLINK = :
STRIP = strip
INS = install
IFILE = lbld
MAPFILE = boot.map
LIBFM = libfm.a
FRC =

FILES = \
	misc.o \
	boot.o \
	strrchr.o \
	strncat.o

all: boot

boot: $(FILES) $(IFILE) $(LIBFM) $(MAPFILE) $(FRC)
	if [ x$(CCSTYPE) = xCOFF ] ; \
	then $(LD) $(LDFLAGS) $(IFILE) $(FILES) $(LIBFM) -o boot ; \
	else $(LD) $(LDFLAGS) -M $(MAPFILE) $(FILES) $(LIBFM) -dn -o boot ; \
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
	$(INC)/sys/boot.h \
	$(INC)/sys/csr.h \
	$(INC)/sys/elog.h \
	$(INC)/sys/firmware.h \
	$(INC)/sys/id.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/nvram.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/types.h \
	$(INC)/sys/dma.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/fs/bfs.h \
	$(INC)/sys/fsiboot.h \
	$(FRC)
