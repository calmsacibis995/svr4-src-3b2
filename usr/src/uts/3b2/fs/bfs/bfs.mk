#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)fs:fs/bfs/bfs.mk	1.15"
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

FILES =\
	bfs_compact.o \
	bfs_subr.o \
	bfs_vfsops.o \
	bfs_vnops.o

all:	BFS

BFS:	bfs.o $(MASTERD)/bfs
	$(MKBOOT) -m $(MASTERD) -d . bfs.o

bfs.o: $(FILES)
	$(LD) -r -o bfs.o $(FILES)

.c.o:
	$(CC) $(DEFLIST) -I$(INC) $(CFLAGS) -c $*.c

clean:
	-rm -f *.o

clobber:	clean
	-rm -f bfs BFS

#
# Header dependencies
#

bfs_compact.o: bfs_compact.c \
	$(INC)/sys/types.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/conf.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/file.h \
	$(INC)/sys/flock.h \
	$(INC)/sys/inline.h \
	$(INC)/sys/open.h \
	$(INC)/sys/param.h \
	$(INC)/sys/pathname.h \
	$(INC)/sys/sysinfo.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/uio.h \
	$(INC)/sys/var.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/kmem.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/user.h \
	$(INC)/sys/fs/bfs.h \
	$(INC)/sys/fs/bfs_compact.h \
	$(FRC)

bfs_subr.o: bfs_subr.c \
	$(INC)/sys/types.h \
	$(INC)/sys/time.h \
	$(INC)/sys/param.h \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/var.h \
	$(INC)/sys/kmem.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/uio.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/fs/bfs.h \
	$(INC)/sys/flock.h \
	$(FRC)

bfs_vfsops.o: bfs_vfsops.c \
	$(INC)/sys/types.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/conf.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/file.h \
	$(INC)/sys/mount.h \
	$(INC)/sys/open.h \
	$(INC)/sys/param.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/statvfs.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/uio.h \
	$(INC)/sys/var.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/kmem.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/user.h \
	$(INC)/sys/fs/bfs.h \
	$(INC)/fs/fs_subr.h \
	$(FRC)

bfs_vnops.o: bfs_vnops.c \
	$(INC)/sys/types.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/conf.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/time.h \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/file.h \
	$(INC)/sys/flock.h \
	$(INC)/sys/inline.h \
	$(INC)/sys/open.h \
	$(INC)/sys/param.h \
	$(INC)/sys/pathname.h \
	$(INC)/sys/sysinfo.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/uio.h \
	$(INC)/sys/var.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/kmem.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/user.h \
	$(INC)/sys/debug.h \
	$(INC)/vm/seg.h \
	$(INC)/vm/page.h \
	$(INC)/vm/pvn.h \
	$(INC)/sys/mman.h \
	$(INC)/sys/fs/bfs.h \
	$(INC)/fs/fs_subr.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/disp.h \
	$(FRC)

