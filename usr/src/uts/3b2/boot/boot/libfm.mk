#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)boot:boot/boot/libfm.mk	1.1"

ROOT =
INC = $(ROOT)/usr/include
DASHO = -O
CFLAGS = $(DASHO) -I$(INC) -I .
SYMLINK = :
INS = install
LIBFM = libfm.a
FRC =

FILES =\
	bfslibfm.o \
	3b2_io.o \
	hdacs.o \
	hdmap.o \
	hdmisc.o \
	ddma.o

all: libfm.a

libfm.a: $(FILES) 
	$(AR) ruv $(LIBFM) $(FILES)

install: 

clean:
	-rm -f *.o

clobber: clean
	-rm -f libfm.a

#
# Header dependencies
#

bfs.o: bfs.c \
	$(INC)/sys/types.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/elog.h \
	$(INC)/sys/iobuf.h \
	$(INC)/sys/boot.h \
	$(INC)/sys/firmware.h \
	$(INC)/sys/id.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/csr.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/nvram.h \
	$(INC)/sys/vtoc.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/fsiboot.h \
	$(INC)/sys/fs/bfs.h \
	$(FRC)

3b2_io.o: 3b2_io.c \
	$(INC)/sys/types.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/elog.h \
	$(INC)/sys/iobuf.h \
	$(INC)/sys/boot.h \
	$(INC)/sys/firmware.h \
	$(INC)/sys/id.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/csr.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/nvram.h \
	$(INC)/sys/vtoc.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/fsiboot.h \
	$(INC)/sys/fs/bfs.h \
	$(FRC)

hdmisc.o: hdmisc.c \
	$(INC)/sys/types.h \
	$(INC)/sys/id.h \
	$(INC)/sys/vtoc.h \
	$(FRC)

hdacs.o: hdacs.c \
	$(INC)/sys/firmware.h \
	$(INC)/sys/dma.h \
	$(INC)/sys/types.h \
	$(INC)/sys/vtoc.h \
	$(INC)/sys/id.h \
	$(FRC)

hdmap.o: hdmap.c \
	$(INC)/sys/sbd.h \
	$(INC)/sys/firmware.h \
	$(INC)/sys/types.h \
	$(INC)/sys/vtoc.h \
	$(INC)/sys/id.h \
	$(INC)/sys/ertyp.h \
	$(INC)/sys/fsiboot.h \
	$(FRC)

ddma.o: ddma.c \
	$(INC)/sys/sbd.h \
	$(INC)/sys/firmware.h \
	$(INC)/sys/dma.h \
	$(FRC)

misc.o: misc.s \
	$(FRC)

