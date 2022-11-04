#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)npack:io/npack.mk	1.8"
ROOT =
INC = $(ROOT)/usr/include

MACHINE = 3b2

MORECPP =
MKBOOT = mkboot
CFLAGS = -O -I.. -I$(INC) $(MORECPP) -Du$(MACHINE) -D_KERNEL 
LDFLAGS =
CC = cc
LD = ld

CLEAN =	npack.o

.c.o:
	$(CC) $(CFLAGS) -c $*.c
	$(MKBOOT) -m ../master.d -d . npack.o

all : npack.o

npack.o: npack.c \
	../master.d/npack \
	../sys/npack.h \
	$(INC)/sys/types.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/param.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/strlog.h \
	$(INC)/sys/log.h \
	$(INC)/sys/tihdr.h \
	$(INC)/sys/tiuser.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/user.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/dlpi.h \
	$(INC)/sys/rf_debug.h

install: all

clean:
		rm -f $(CLEAN)

clobber:	clean
