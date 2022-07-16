#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)fs:fs/s5/s5.mk	1.15"
ROOT =
STRIP = strip
INC = $(ROOT)/usr/include
MKBOOT = mkboot
MASTERD = ../../master.d

DASHG =
DASHO = -O
PFLAGS = $(DASHG) -D_KERNEL $(MORECPP)
CFLAGS = $(DASHO) $(PFLAGS)
DEFLIST =
FRC =

FILES = \
	s5alloc.o \
	s5blklist.o \
	s5bmap.o \
	s5dir.o \
	s5getsz.o \
	s5inode.o \
	s5rdwri.o \
	s5vfsops.o \
	s5vnops.o

all:	S5

S5:	s5.o $(MASTERD)/s5
	$(MKBOOT) -m $(MASTERD) -d . s5.o

s5.o: $(FILES)
	$(LD) -r -o s5.o $(FILES)

.c.o:
	$(CC) $(DEFLIST) -I$(INC) $(CFLAGS) -c $*.c

clean:
	-rm -f *.o

clobber:	clean
	-rm -f s5 S5

#
# Header dependencies
#

s5alloc.o: s5alloc.c \
	$(INC)/sys/types.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/conf.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/file.h \
	$(INC)/sys/flock.h \
	$(INC)/sys/param.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/var.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/mode.h \
	$(INC)/sys/user.h \
	$(INC)/vm/pvn.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/disp.h \
	$(INC)/sys/fs/s5param.h \
	$(INC)/sys/fs/s5fblk.h \
	$(INC)/sys/fs/s5filsys.h \
	$(INC)/sys/fs/s5ino.h \
	$(INC)/sys/fs/s5inode.h \
	$(INC)/sys/fs/s5macros.h \
	$(INC)/fs/fs_subr.h \
	$(FRC)

s5blklist.o: s5blklist.c \
	$(INC)/sys/types.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/conf.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/file.h \
	$(INC)/sys/flock.h \
	$(INC)/sys/param.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/var.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/mode.h \
	$(INC)/sys/user.h \
	$(INC)/vm/pvn.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/disp.h \
	$(INC)/sys/fs/s5param.h \
	$(INC)/sys/fs/s5fblk.h \
	$(INC)/sys/fs/s5filsys.h \
	$(INC)/sys/fs/s5ino.h \
	$(INC)/sys/fs/s5inode.h \
	$(INC)/sys/fs/s5macros.h \
	$(INC)/fs/fs_subr.h \
	$(FRC)

s5bmap.o: s5bmap.c \
	$(INC)/sys/types.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/fbuf.h \
	$(INC)/sys/file.h \
	$(INC)/sys/param.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/disp.h \
	$(INC)/sys/fs/s5param.h \
	$(INC)/sys/fs/s5inode.h \
	$(INC)/sys/fs/s5macros.h \
	$(INC)/vm/seg.h \
	$(FRC)

s5dir.o: s5dir.c \
	$(INC)/sys/types.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/fbuf.h \
	$(INC)/sys/file.h \
	$(INC)/sys/param.h \
	$(INC)/sys/pathname.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/sysinfo.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/uio.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/mode.h \
	$(INC)/sys/dnlc.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/disp.h \
	$(INC)/sys/fs/s5param.h \
	$(INC)/sys/fs/s5dir.h \
	$(INC)/sys/fs/s5inode.h \
	$(INC)/sys/fs/s5macros.h \
	$(INC)/vm/seg.h \
	$(FRC)

s5getsz.o: s5getsz.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/disp.h \
	$(INC)/sys/fs/s5param.h \
	$(INC)/sys/fs/s5fblk.h \
	$(INC)/sys/fs/s5filsys.h \
	$(INC)/sys/fs/s5ino.h \
	$(INC)/sys/fs/s5inode.h \
	$(INC)/sys/fs/s5macros.h \
	$(FRC)
	$(CC) -I$(INC) -g $(PFLAGS) -c $<

s5inode.o: s5inode.c \
	$(INC)/sys/types.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/conf.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/file.h \
	$(INC)/sys/kmem.h \
	$(INC)/sys/open.h \
	$(INC)/sys/param.h \
	$(INC)/sys/time.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/swap.h \
	$(INC)/sys/sysinfo.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/var.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/mode.h \
	$(INC)/sys/dnlc.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/disp.h \
	$(INC)/sys/fs/s5param.h \
	$(INC)/sys/fs/s5dir.h \
	$(INC)/sys/fs/s5ino.h \
	$(INC)/sys/fs/s5inode.h \
	$(INC)/sys/fs/s5macros.h \
	$(INC)/vm/page.h \
	$(INC)/vm/pvn.h \
	$(INC)/fs/fs_subr.h \
	$(FRC)

s5rdwri.o: s5rdwri.c \
	$(INC)/sys/types.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/conf.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/file.h \
	$(INC)/sys/param.h \
	$(INC)/sys/swap.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/uio.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/disp.h \
	$(INC)/sys/fs/s5param.h \
	$(INC)/sys/fs/s5inode.h \
	$(INC)/sys/fs/s5macros.h \
	$(INC)/vm/seg_kmem.h \
	$(INC)/vm/seg_map.h \
	$(INC)/vm/seg.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/kmem.h \
	$(FRC)

s5vfsops.o: s5vfsops.c \
	$(INC)/sys/types.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/conf.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/fbuf.h \
	$(INC)/sys/file.h \
	$(INC)/sys/kmem.h \
	$(INC)/sys/mount.h \
	$(INC)/sys/open.h \
	$(INC)/sys/param.h \
	$(INC)/sys/time.h \
	$(INC)/sys/statvfs.h \
	$(INC)/sys/swap.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/uio.h \
	$(INC)/sys/var.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/user.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/disp.h \
	$(INC)/vm/seg.h \
	$(INC)/sys/fs/s5param.h \
	$(INC)/sys/fs/s5macros.h \
	$(INC)/sys/fs/s5inode.h \
	$(INC)/sys/fs/s5filsys.h \
	$(INC)/fs/fs_subr.h \
	$(FRC)

s5vnops.o: s5vnops.c \
	$(INC)/sys/types.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/conf.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/dirent.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/fbuf.h \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/file.h \
	$(INC)/sys/flock.h \
	$(INC)/sys/kmem.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/inline.h \
	$(INC)/sys/mman.h \
	$(INC)/sys/open.h \
	$(INC)/sys/param.h \
	$(INC)/sys/time.h \
	$(INC)/sys/pathname.h \
	$(INC)/sys/sysinfo.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/uio.h \
	$(INC)/sys/var.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/disp.h \
	$(INC)/sys/user.h \
	$(INC)/sys/fs/s5param.h \
	$(INC)/sys/fs/s5dir.h \
	$(INC)/sys/fs/s5filsys.h \
	$(INC)/sys/fs/s5inode.h \
	$(INC)/sys/fs/s5macros.h \
	$(INC)/vm/page.h \
	$(INC)/vm/pvn.h \
	$(INC)/vm/as.h \
	$(INC)/vm/seg.h \
	$(INC)/vm/seg_map.h \
	$(INC)/vm/seg_vn.h \
	$(INC)/vm/rm.h \
	$(INC)/fs/fs_subr.h \
	$(FRC)
