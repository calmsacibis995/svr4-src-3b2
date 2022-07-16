#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)fs:fs/xnamfs/xnamfs.mk	1.4"
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

FILES = \
	xnamgetsizes.o \
	xnamsubr.o \
	xnamvfsops.o \
	xnamvnops.o \
	xsd.o \
	xsem.o

all:	XNAMFS

XNAMFS:	xnamfs.o $(MASTERD)/xnamfs
	$(MKBOOT) -m $(MASTERD) -d . xnamfs.o

xnamfs.o: $(FILES)
	$(LD) -r -o xnamfs.o $(FILES)

.c.o:
	$(CC) $(DEFLIST) -I$(INC) $(CFLAGS) -c $*.c

clean:
	-rm -f *.o

clobber:	clean
	-rm -f xnamfs XNAMFS

#
# Header dependencies
#

xnamgetsizes.o: xnamgetsizes.c \
	$(INC)/sys/types.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/fs/xnamnode.h \
	$(FRC)
	$(CC) $(PFLAGS) -I$(INC) -c -g $<

xnamsubr.o: xnamsubr.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/conf.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/kmem.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/fs/xnamnode.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/time.h \
	$(INC)/sys/file.h \
	$(FRC)

xnamvfsops.o: xnamvfsops.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/swap.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/fs/xnamnode.h \
	$(INC)/fs/fs_subr.h \
	$(FRC)

xnamvnops.o: xnamvnops.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/conf.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/time.h \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/flock.h \
	$(INC)/sys/file.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/kmem.h \
	$(INC)/sys/mman.h \
	$(INC)/sys/open.h \
	$(INC)/sys/swap.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/uio.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/poll.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/user.h \
	$(INC)/sys/fs/xnamnode.h \
	$(INC)/vm/seg.h \
	$(INC)/vm/seg_map.h \
	$(INC)/vm/page.h \
	$(INC)/vm/pvn.h \
	$(INC)/vm/seg_dev.h \
	$(INC)/vm/seg_vn.h \
	$(INC)/fs/fs_subr.h \
	$(FRC)

xsem.o: xsem.c \
	$(INC)/sys/types.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/param.h \
	$(INC)/sys/dir.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/user.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/file.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/fs/xnamnode.h \
	$(INC)/sys/var.h \
	$(INC)/sys/conf.h \
	$(INC)/sys/fstyp.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/rf_messg.h \
	$(INC)/sys/uio.h \
	$(INC)/sys/sysinfo.h \
	$(FRC)

xsd.o: xsd.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/dir.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/user.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/vm.h \
	$(INC)/sys/sd.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/tuneable.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/var.h \
	$(INC)/sys/fstyp.h \
	$(INC)/sys/conf.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/fs/xnamnode.h \
	$(INC)/sys/debug.h \
	$(FRC)

