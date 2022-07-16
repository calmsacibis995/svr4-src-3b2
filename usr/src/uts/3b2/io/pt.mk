#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kernel:io/pt.mk	1.6"
ROOT =
STRIP = strip
INC = $(ROOT)/usr/include
MKBOOT = mkboot
MASTERD = ../master.d

DASHO = -O 
PFLAGS= -I$(INC) -D_KERNEL $(MORECPP)
CFLAGS= $(DASHO) -UDBUG $(PFLAGS)
FRC =

DFILES = ptm.o pts.o ptem.o pckt.o

drivers:	all

all:	PTM PTS PTEM PCKT

clean:
	-rm -f $(DFILES)

clobber:	clean
	-rm -f PTM PTS PTEM PCKT

FRC:

PTM:	ptm.o $(MASTERD)/ptm
	$(MKBOOT) -m $(MASTERD) -d . ptm.o;

PTS:	pts.o $(MASTERD)/pts
	$(MKBOOT) -m $(MASTERD) -d . pts.o;

PCKT:	pckt.o $(MASTERD)/pckt
	$(MKBOOT) -m $(MASTERD) -d . pckt.o

PTEM:	ptem.o $(MASTERD)/ptem
	$(MKBOOT) -m $(MASTERD) -d . ptem.o

#
# Header dependencies
#

ptm.o: ptm.c \
	$(INC)/sys/types.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/param.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/user.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/ptms.h \
	$(FRC)

pts.o: pts.c \
	$(INC)/sys/types.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/param.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/user.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/ptms.h \
	$(FRC)

ptem.o: ptem.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/termio.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/user.h \
	$(INC)/sys/strtty.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/jioctl.h \
	$(INC)/sys/ptem.h \
	$(INC)/sys/debug.h \
	$(FRC)

pckt.o: pckt.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/user.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/debug.h \
	$(FRC)

