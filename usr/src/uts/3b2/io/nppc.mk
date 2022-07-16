#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kernel:io/nppc.mk	1.11"

ROOT =
STRIP = strip
INC = $(ROOT)/usr/include
MKBOOT = mkboot
MASTERD = ../master.d

DASHO = -O
PFLAGS = -I$(INC) -D_KERNEL $(MORECPP)
CFLAGS = $(DASHO) $(PFLAGS)
FRC =

DFILES = \
	ports.o

OPFILES = \
	lla_ppc

all:	PORTS

clean:
	-rm -f nppc.o lla_ppc.o ports.o

clobber:	clean
	-rm -f PORTS

ports.o:	lla_ppc.o nppc.o
	-$(LD) -r nppc.o lla_ppc.o -o ports.o

FRC:

PORTS:	$(DFILES) $(MASTERD)/ports
	$(MKBOOT) -m $(MASTERD) -d . ports.o

#
# Header dependencies
#

nppc.o: nppc.c \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/iu.h \
	$(INC)/sys/file.h \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/termio.h \
	$(INC)/sys/conf.h \
	$(INC)/sys/firmware.h \
	$(INC)/sys/devcode.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/strpump.h \
	$(INC)/sys/cio_defs.h \
	$(INC)/sys/pp_dep.h \
	$(INC)/sys/queue.h \
	$(INC)/sys/strlla_ppc.h \
	$(INC)/sys/ppc_lla.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/strtty.h \
	$(INC)/sys/strppc.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/eucioctl.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/ddi.h \
	$(FRC)

lla_ppc.o: lla_ppc.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/dir.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/user.h \
	$(INC)/sys/pp_dep.h \
	$(INC)/sys/queue.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/cio_defs.h \
	$(INC)/sys/ppc_lla.h \
	$(INC)/sys/strlla_ppc.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/strppc.h \
	$(INC)/sys/termio.h \
	$(INC)/sys/strtty.h \
	$(FRC)

