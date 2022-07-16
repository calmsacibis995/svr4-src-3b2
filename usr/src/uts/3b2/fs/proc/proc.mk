#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)fs:fs/proc/proc.mk	1.20"

ROOT = 
STRIP = strip
INC = $(ROOT)/usr/include
MKBOOT = mkboot
MASTERD = ../../master.d
CC = cc

DASHG = 
DASHO = -O
PFLAGS = $(DASHG) -D_KERNEL $(MORECPP)
CFLAGS = $(DASHO) $(PFLAGS)
DEFLIST =
FRC =

FILES = prioctl.o prmachdep.o prsubr.o prusrio.o prvfsops.o prvnops.o
all:	PROC

PROC:	proc.o $(MASTERD)/proc
	$(MKBOOT) -m $(MASTERD) -d . proc.o

proc.o: $(FILES)
	$(LD) -r -o proc.o $(FILES)

.c.o:
	$(CC) $(DEFLIST) -I$(INC) $(CFLAGS) -c $*.c

lint:
	lint -u -x $(DEFLIST) -I$(INC) $(CFLAGS) \
		prioctl.c prmachdep.c prsubr.c prusrio.c prvfsops.c prvnops.c

clean:
	-rm -f *.o

clobber:	clean
	-rm -f PROC

#
# Header dependencies
#
prioctl.o: prioctl.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/file.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/inline.h \
	$(INC)/sys/kmem.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/reg.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/user.h \
	$(INC)/sys/fault.h \
	$(INC)/sys/syscall.h \
	$(INC)/sys/procfs.h \
	$(INC)/vm/as.h \
	$(INC)/vm/seg.h \
	prdata.h \
	$(FRC)

prmachdep.o: prmachdep.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/inline.h \
	$(INC)/sys/kmem.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/reg.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/user.h \
	$(INC)/sys/fault.h \
	$(INC)/sys/syscall.h \
	$(INC)/sys/procfs.h \
	$(INC)/vm/as.h \
	$(INC)/vm/page.h \
	prdata.h \
	$(FRC)

prsubr.o: prsubr.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/inline.h \
	$(INC)/sys/kmem.h \
	$(INC)/sys/mman.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/reg.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/uio.h \
	$(INC)/sys/var.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/session.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/user.h \
	$(INC)/sys/disp.h \
	$(INC)/sys/class.h \
	$(INC)/sys/ts.h \
	$(INC)/sys/bitmap.h \
	$(INC)/sys/fault.h \
	$(INC)/sys/syscall.h \
	$(INC)/sys/procfs.h \
	$(INC)/vm/as.h \
	$(INC)/vm/rm.h \
	$(INC)/vm/seg.h \
	prdata.h \
	$(FRC)

prusrio.o: prusrio.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/inline.h \
	$(INC)/sys/mman.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/procfs.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/uio.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/user.h \
	$(INC)/vm/as.h \
	$(INC)/vm/seg.h \
	prdata.h \
	$(FRC)

prvfsops.o: prvfsops.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/inline.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/procfs.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/statvfs.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/var.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/mode.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/user.h \
	$(INC)/fs/fs_subr.h \
	prdata.h \
	$(FRC)

prvnops.o: prvnops.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/time.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/dirent.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/file.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/inline.h \
	$(INC)/sys/kmem.h \
	$(INC)/sys/pathname.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/uio.h \
	$(INC)/sys/var.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/mode.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/user.h \
	$(INC)/sys/fault.h \
	$(INC)/sys/syscall.h \
	$(INC)/sys/procfs.h \
	$(INC)/fs/fs_subr.h \
	$(INC)/vm/rm.h \
	prdata.h \
	$(FRC)
