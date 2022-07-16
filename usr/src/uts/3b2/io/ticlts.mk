#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kernel:io/ticlts.mk	1.4"
ROOT =
STRIP = strip
INC = $(ROOT)/usr/include
MKBOOT = mkboot
MASTERD = ../master.d

DASHO = -O
PFLAGS = -I$(INC) -D_KERNEL $(MORECPP)
CFLAGS = $(DASHO) $(PFLAGS)
FRC =

DFILES = ticlts.o

drivers: all

all: TICLTS

clean:
	-rm -f $(DFILES)

clobber: clean
	-rm -f TICLTS

FRC:

TICLTS: ticlts.o $(MASTERD)/ticlts
	$(MKBOOT) -m $(MASTERD) -d . ticlts.o;

ticlts.o: ticlts.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/tihdr.h \
	$(INC)/sys/tiuser.h \
	$(INC)/sys/strlog.h \
	$(INC)/sys/log.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/user.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/kmem.h \
 	$(INC)/sys/mkdev.h \
	$(INC)/sys/ticlts.h \
	$(FRC)
