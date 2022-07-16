#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kernel:disp/disp.mk	1.8"
ROOT =
STRIP = strip
INC = $(ROOT)/usr/include
MKBOOT = mkboot
MASTERD = ../master.d

DASHG =
DASHO = -O
PFLAGS = $(DASHG) -D_KERNEL $(MORECPP)
CFLAGS = $(DASHO) $(PFLAGS)
DEFLIST =
FRC =

FILES = \
	disp.o \
	priocntl.o \
	sysclass.o

all:	gendisp.o classes

clean:
	-rm -f $(FILES) rt.o ts.o

clobber:	clean
	-rm -f gendisp.o RT TS

gendisp.o: $(FILES)
	$(LD) -r -o gendisp.o $(FILES)

classes:	RT \
		TS

RT:	rt.o $(MASTERD)/rt
	$(MKBOOT) -m $(MASTERD) -d . rt.o

TS:	ts.o $(MASTERD)/ts
	$(MKBOOT) -m $(MASTERD) -d . ts.o

.c.o:
	$(CC) $(DEFLIST) -I$(INC) $(CFLAGS) -c $*.c

#
# Header dependencies
#

disp.o: disp.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/fs/s5dir.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/user.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/sysinfo.h \
	$(INC)/sys/var.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/procset.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/inline.h \
	$(INC)/sys/priocntl.h \
	$(INC)/sys/disp.h \
	$(INC)/sys/class.h \
	$(INC)/sys/bitmap.h \
	$(INC)/sys/kmem.h \
	$(INC)/vm/vm_hat.h \
	$(INC)/vm/as.h \
	$(FRC)

priocntl.o: priocntl.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/fs/s5dir.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/user.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/sysinfo.h \
	$(INC)/sys/var.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/procset.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/inline.h \
	$(INC)/sys/priocntl.h \
	$(INC)/sys/disp.h \
	$(INC)/sys/class.h \
	$(INC)/sys/ts.h \
	$(INC)/sys/tspriocntl.h \
	$(FRC)

rt.o: rt.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/fs/s5dir.h \
	$(INC)/sys/user.h \
	$(INC)/sys/evecb.h \
	$(INC)/sys/hrtcntl.h \
	$(INC)/sys/priocntl.h \
	$(INC)/sys/class.h \
	$(INC)/sys/disp.h \
	$(INC)/sys/procset.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/rt.h \
	$(INC)/sys/rtpriocntl.h \
	$(INC)/sys/kmem.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/inline.h \
	$(INC)/sys/errno.h \
	$(FRC)

sysclass.o: sysclass.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/fs/s5dir.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/user.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/sysinfo.h \
	$(INC)/sys/var.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/inline.h \
	$(INC)/sys/disp.h \
	$(INC)/sys/class.h \
	$(FRC)

ts.o: ts.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/fs/s5dir.h \
	$(INC)/sys/user.h \
	$(INC)/sys/priocntl.h \
	$(INC)/sys/class.h \
	$(INC)/sys/disp.h \
	$(INC)/sys/procset.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/ts.h \
	$(INC)/sys/tspriocntl.h \
	$(INC)/sys/evecb.h \
	$(INC)/sys/hrtcntl.h \
	$(INC)/sys/kmem.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/inline.h \
	$(INC)/sys/errno.h \
	$(FRC)

