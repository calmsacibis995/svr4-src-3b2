#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kernel:io/hrt.mk	1.5"
ROOT =
STRIP = strip
INC = $(ROOT)/usr/include
MKBOOT = mkboot
MASTERD = ../master.d

DASHG = 
DASHO = -O 
PFLAGS = $(DASHG) -I$(INC) -D_KERNEL $(MORECPP)
CFLAGS = $(DASHO) $(PFLAGS)
FRC =

DFILES = \
	hrtimers.o \
	ladd.o \
	lsub.o \
	lmul.o \
	ldivide.o \
	lshiftl.o \
	lsign.o

all:	HRT

clean:
	-rm -f $(DFILES)

clobber:	clean
	-rm -f HRT

HRT:	$(DFILES) $(MASTERD)/hrt
	$(LD) -r -o hrt.o $(DFILES)
	$(MKBOOT) -m $(MASTERD) -d . hrt.o;

#
# Header dependencies
#

hrtimers.o: hrtimers.c \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/sysinfo.h \
	$(INC)/sys/callo.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/sit.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/user.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/inline.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/evecb.h \
	$(INC)/sys/hrtcntl.h \
	$(INC)/sys/dl.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/priocntl.h \
	$(INC)/sys/map.h \
	$(INC)/sys/file.h \
	$(INC)/sys/events.h \
	$(INC)/sys/evsys.h \
	$(INC)/sys/hrtsys.h \
	$(INC)/sys/cmn_err.h \
	$(FRC)

ladd.o: ladd.s \
	$(FRC)

lsub.o: lsub.s \
	$(FRC)

lmul.o: lmul.c \
	$(INC)/sys/types.h \
	$(INC)/sys/dl.h \
	$(FRC)

ldivide.o: ldivide.c \
	$(INC)/sys/types.h \
	$(INC)/sys/dl.h \
	$(FRC)

lshiftl.o: lshiftl.s \
	$(FRC)

lsign.o: lsign.s \
	$(FRC)

