#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kernel:io/lib.io.mk	1.10"
ROOT =
STRIP = strip
INC = $(ROOT)/usr/include

LIBNAME = ../lib.io
DASHO = -O
CFLAGS = $(DASHO) -I$(INC) -D_KERNEL $(MORECPP)
FRC =

FILES = \
	$(LIBNAME)(partab.o)\
	$(LIBNAME)(stream.o)\
	$(LIBNAME)(physdsk.o)

all:	$(LIBNAME)

.PRECIOUS:	$(LIBNAME)

$(LIBNAME):	$(FILES)

clean:
	-rm -f *.o

clobber:	clean
	-rm -f $(LIBNAME)

FRC:
	-rm -f $(LIBNAME)
	$(AR) qc $(LIBNAME)

.c.a:
	$(CC) -c $(CFLAGS) $<;\
	$(AR) rv $@ $*.o
	-rm -f $*.o

#
# Header dependencies
#

$(LIBNAME)(partab.o): partab.c \
	$(FRC)

$(LIBNAME)(stream.o): stream.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/strsubr.h \
	$(INC)/sys/strstat.h \
	$(INC)/sys/conf.h \
	$(INC)/sys/var.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/inline.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/tuneable.h \
	$(INC)/sys/map.h \
	$(INC)/sys/cmn_err.h \
	$(FRC)

$(LIBNAME)(physdsk.o):	physdsk.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/fs/s5dir.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/conf.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/user.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/elog.h \
	$(INC)/sys/iobuf.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/inline.h \
	$(FRC)
