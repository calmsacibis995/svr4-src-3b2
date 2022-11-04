#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)fs:fs/ufs/ufs.mk	1.10"

ROOT =
STRIP = strip
INC = $(ROOT)/usr/include
MKBOOT = mkboot
MASTERD = ../../master.d

DASHG =
DASHO = -O
PFLAGS = $(DASHG) -DQUOTA -D_KERNEL $(MORECPP)
CFLAGS = $(DASHO) $(PFLAGS) -I$(INC)
DEFLIST =
FRC =

FILES = \
	quota.o \
	quotacalls.o \
	quota_ufs.o \
	ufs_alloc.o \
	ufs_bmap.o \
	ufs_blklist.o \
	ufs_dir.o \
	ufs_dsort.o \
	ufs_inode.o \
	ufs_subr.o \
	ufs_tables.o \
	ufs_vfsops.o \
	ufs_vnops.o

all:	UFS

UFS:	ufs.o $(MASTERD)/ufs
	$(MKBOOT) -m $(MASTERD) -d . ufs.o

ufs.o: $(FILES)
	$(LD) -r -o ufs.o $(FILES)

.c.o:
	$(CC) $(DEFLIST) -I$(INC1) $(CFLAGS) -c $*.c

clean:
	-rm -f *.o

clobber:	clean
	-rm -f ufs UFS

#
# Header dependencies
#

quota.o: quota.c \
        $(INC)/sys/types.h \
        $(INC)/sys/param.h \
        $(INC)/sys/systm.h \
        $(INC)/sys/signal.h \
        $(INC)/sys/errno.h \
        $(INC)/sys/user.h \
        $(INC)/sys/proc.h \
        $(INC)/sys/vfs.h \
        $(INC)/sys/vnode.h \
        $(INC)/sys/uio.h \
        $(INC)/sys/fs/ufs_quota.h \
        $(INC)/sys/fs/ufs_inode.h \
        $(INC)/sys/fs/ufs_fs.h \
        $(INC)/sys/cmn_err.h \
        $(INC)/sys/kmem.h
	${CC} -c ${CFLAGS} quota.c

quota.L: quota.c \
        $(INC)/sys/types.h \
        $(INC)/sys/param.h \
        $(INC)/sys/systm.h \
        $(INC)/sys/signal.h \
        $(INC)/sys/errno.h \
        $(INC)/sys/user.h \
        $(INC)/sys/proc.h \
        $(INC)/sys/vfs.h \
        $(INC)/sys/vnode.h \
        $(INC)/sys/uio.h \
        $(INC)/sys/fs/ufs_quota.h \
        $(INC)/sys/fs/ufs_inode.h \
        $(INC)/sys/fs/ufs_fs.h \
        $(INC)/sys/cmn_err.h \
        $(INC)/sys/kmem.h
	@echo quota.c:
	@-(${CPP} ${LCOPTS} quota.c | \
	  ${LINT1} ${LOPTS} > quota.L ) 2>&1 | ${LTAIL}

quotacalls.o: quotacalls.c \
        $(INC)/sys/types.h \
        $(INC)/sys/param.h \
        $(INC)/sys/systm.h \
        $(INC)/sys/signal.h \
        $(INC)/sys/cred.h \
        $(INC)/sys/proc.h \
        $(INC)/sys/user.h \
        $(INC)/sys/proc.h \
        $(INC)/sys/vfs.h \
        $(INC)/sys/vnode.h \
        $(INC)/sys/uio.h \
        $(INC)/sys/fs/ufs_quota.h \
        $(INC)/sys/fs/ufs_inode.h \
        $(INC)/sys/fs/ufs_fs.h \
        $(INC)/sys/errno.h
	${CC} -c ${CFLAGS} quotacalls.c

quotacalls.L: quotacalls.c \
        $(INC)/sys/types.h \
        $(INC)/sys/param.h \
        $(INC)/sys/systm.h \
        $(INC)/sys/signal.h \
        $(INC)/sys/cred.h \
        $(INC)/sys/proc.h \
        $(INC)/sys/user.h \
        $(INC)/sys/proc.h \
        $(INC)/sys/vfs.h \
        $(INC)/sys/vnode.h \
        $(INC)/sys/uio.h \
        $(INC)/sys/fs/ufs_quota.h \
        $(INC)/sys/fs/ufs_inode.h \
        $(INC)/sys/fs/ufs_fs.h \
        $(INC)/sys/errno.h \
#       $(INC)/sys/quota.h
	@echo quotacalls.c:
	@-(${CPP} ${LCOPTS} quotacalls.c | \
	  ${LINT1} ${LOPTS} > quotacalls.L ) 2>&1 | ${LTAIL}

quota_ufs.o: quota_ufs.c \
        $(INC)/sys/types.h \
        $(INC)/sys/param.h \
        $(INC)/sys/systm.h \
        $(INC)/sys/signal.h \
        $(INC)/sys/cred.h \
        $(INC)/sys/proc.h \
        $(INC)/sys/user.h \
        $(INC)/sys/proc.h \
        $(INC)/sys/vfs.h \
        $(INC)/sys/vnode.h \
        $(INC)/sys/buf.h \
        $(INC)/sys/uio.h \
        $(INC)/sys/fs/ufs_quota.h \
        $(INC)/sys/fs/ufs_inode.h \
        $(INC)/sys/fs/ufs_fs.h \
        $(INC)/sys/errno.h
	${CC} -c ${CFLAGS} quota_ufs.c

quota_ufs.L: quota_ufs.c \
        $(INC)/sys/types.h \
        $(INC)/sys/param.h \
        $(INC)/sys/systm.h \
        $(INC)/sys/signal.h \
        $(INC)/sys/cred.h \
        $(INC)/sys/proc.h \
        $(INC)/sys/user.h \
        $(INC)/sys/proc.h \
        $(INC)/sys/vfs.h \
        $(INC)/sys/vnode.h \
        $(INC)/sys/buf.h \
        $(INC)/sys/uio.h \
        $(INC)/sys/fs/ufs_quota.h \
        $(INC)/sys/fs/ufs_inode.h \
        $(INC)/sys/fs/ufs_fs.h \
        $(INC)/sys/errno.h
	@echo quota_ufs.c:
	@-(${CPP} ${LCOPTS} quota_ufs.c | \
	  ${LINT1} ${LOPTS} > quota_ufs.L ) 2>&1 | ${LTAIL}

ufs_alloc.o: ufs_alloc.c \
        $(INC)/sys/types.h \
        $(INC)/sys/param.h \
        $(INC)/sys/systm.h \
        $(INC)/sys/signal.h \
        $(INC)/sys/cred.h \
        $(INC)/sys/proc.h \
        $(INC)/sys/user.h \
        $(INC)/sys/buf.h \
        $(INC)/sys/vfs.h \
        $(INC)/sys/vnode.h \
        $(INC)/sys/fs/ufs_fs.h \
        $(INC)/sys/fs/ufs_inode.h \
        $(INC)/sys/fs/ufs_quota.h \
        $(INC)/sys/errno.h \
	$(INC)/sys/time.h \
        $(INC)/sys/sysmacros.h \
	$(INC)/fs/fs_subr.h
	${CC} -c ${CFLAGS} ufs_alloc.c
 
ufs_alloc.L: ufs_alloc.c \
        $(INC)/sys/types.h \
        $(INC)/sys/param.h \
        $(INC)/sys/systm.h \
        $(INC)/sys/signal.h \
        $(INC)/sys/cred.h \
        $(INC)/sys/proc.h \
        $(INC)/sys/user.h \
        $(INC)/sys/buf.h \
        $(INC)/sys/vfs.h \
        $(INC)/sys/vnode.h \
        $(INC)/sys/fs/ufs_fs.h \
        $(INC)/sys/fs/ufs_inode.h \
        $(INC)/sys/fs/ufs_quota.h \
        $(INC)/sys/errno.h \
 	$(INC)/sys/time.h \
        $(INC)/sys/sysmacros.h \
	$(INC)/fs/fs_subr.h
	@echo ufs_alloc.c:
	@-(${CPP} ${LCOPTS} ufs_alloc.c | \
	  ${LINT1} ${LOPTS} > ufs_alloc.L ) 2>&1 | ${LTAIL}
 
ufs_bmap.o: ufs_bmap.c \
        $(INC)/sys/types.h \
        $(INC)/sys/param.h \
        $(INC)/sys/systm.h \
        $(INC)/sys/signal.h \
        $(INC)/sys/user.h \
        $(INC)/sys/vnode.h \
        $(INC)/sys/buf.h \
        $(INC)/sys/proc.h \
        $(INC)/sys/conf.h \
        $(INC)/sys/fs/ufs_inode.h \
        $(INC)/sys/fs/ufs_fs.h \
        $(INC)/vm/seg.h \
        $(INC)/sys/errno.h \
        $(INC)/sys/sysmacros.h
	${CC} -c ${CFLAGS} ufs_bmap.c
 
ufs_bmap.L: ufs_bmap.c \
        $(INC)/sys/types.h \
        $(INC)/sys/param.h \
        $(INC)/sys/systm.h \
        $(INC)/sys/signal.h \
        $(INC)/sys/user.h \
        $(INC)/sys/vnode.h \
        $(INC)/sys/buf.h \
        $(INC)/sys/proc.h \
        $(INC)/sys/conf.h \
        $(INC)/sys/fs/ufs_inode.h \
        $(INC)/sys/fs/ufs_fs.h \
        $(INC)/vm/seg.h \
        $(INC)/sys/errno.h \
        $(INC)/sys/sysmacros.h
	@echo ufs_bmap.c:
	@-(${CPP} ${LCOPTS} ufs_bmap.c | \
	  ${LINT1} ${LOPTS} > ufs_bmap.L ) 2>&1 | ${LTAIL}
 
ufs_blklist.o: ufs_blklist.c \
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
	$(INC)/sys/fs/ufs_fs.h \
	$(INC)/sys/fs/ufs_inode.h \
	$(INC)/fs/fs_subr.h
	${CC} -c ${CFLAGS} ufs_blklist.c

ufs_dir.o: ufs_dir.c \
        $(INC)/sys/types.h \
        $(INC)/sys/param.h \
        $(INC)/sys/systm.h \
        $(INC)/sys/signal.h \
        $(INC)/sys/cred.h \
        $(INC)/sys/proc.h \
        $(INC)/sys/user.h \
        $(INC)/sys/vfs.h \
        $(INC)/sys/vnode.h \
        $(INC)/sys/stat.h \
        $(INC)/sys/mode.h \
        $(INC)/sys/buf.h \
        $(INC)/sys/uio.h \
        $(INC)/sys/dnlc.h \
        $(INC)/sys/fs/ufs_inode.h \
        $(INC)/sys/fs/ufs_fs.h \
        $(INC)/sys/mount.h \
        $(INC)/sys/fs/ufs_fsdir.h \
        $(INC)/sys/fs/ufs_quota.h \
        $(INC)/sys/errno.h \
        $(INC)/sys/debug.h \
        $(INC)/vm/seg.h \
        $(INC)/sys/sysmacros.h
	${CC} -c ${CFLAGS} ufs_dir.c
 
ufs_dir.L: ufs_dir.c \
        $(INC)/sys/types.h \
        $(INC)/sys/param.h \
        $(INC)/sys/systm.h \
        $(INC)/sys/signal.h \
        $(INC)/sys/cred.h \
        $(INC)/sys/proc.h \
        $(INC)/sys/user.h \
        $(INC)/sys/vfs.h \
        $(INC)/sys/vnode.h \
        $(INC)/sys/stat.h \
        $(INC)/sys/mode.h \
        $(INC)/sys/buf.h \
        $(INC)/sys/uio.h \
        $(INC)/sys/dnlc.h \
        $(INC)/sys/fs/ufs_inode.h \
        $(INC)/sys/fs/ufs_fs.h \
        $(INC)/sys/mount.h \
        $(INC)/sys/fs/ufs_fsdir.h \
        $(INC)/sys/fs/ufs_quota.h \
        $(INC)/sys/errno.h \
        $(INC)/sys/debug.h \
        $(INC)/vm/seg.h \
        $(INC)/sys/sysmacros.h
	@echo ufs_dir.c:
	@-(${CPP} ${LCOPTS} ufs_dir.c | \
	  ${LINT1} ${LOPTS} > ufs_dir.L ) 2>&1 | ${LTAIL}
 
ufs_dsort.o: ufs_dsort.c \
        $(INC)/sys/types.h \
        $(INC)/sys/param.h \
        $(INC)/sys/systm.h \
        $(INC)/sys/buf.h
	${CC} -c ${CFLAGS} ufs_dsort.c
 
ufs_dsort.L: ufs_dsort.c \
        $(INC)/sys/types.h \
        $(INC)/sys/param.h \
        $(INC)/sys/systm.h \
        $(INC)/sys/buf.h
	@echo ufs_dsort.c:
	@-(${CPP} ${LCOPTS} ufs_dsort.c | \
	  ${LINT1} ${LOPTS} > ufs_dsort.L ) 2>&1 | ${LTAIL}
 
ufs_inode.o: ufs_inode.c \
        $(INC)/sys/types.h \
        $(INC)/sys/param.h \
        $(INC)/sys/systm.h \
        $(INC)/sys/signal.h \
        $(INC)/sys/cred.h \
        $(INC)/sys/user.h \
        $(INC)/sys/vfs.h \
        $(INC)/sys/stat.h \
        $(INC)/sys/vnode.h \
        $(INC)/sys/buf.h \
        $(INC)/sys/proc.h \
        $(INC)/sys/mode.h \
        $(INC)/sys/cmn_err.h \
        $(INC)/sys/fs/ufs_inode.h \
        $(INC)/sys/fs/ufs_fs.h \
        $(INC)/sys/fs/ufs_quota.h \
        $(INC)/vm/hat.h \
        $(INC)/vm/as.h \
        $(INC)/vm/pvn.h \
        $(INC)/vm/seg.h \
        $(INC)/sys/swap.h \
        $(INC)/sys/sysinfo.h \
        $(INC)/sys/sysmacros.h \
        $(INC)/sys/errno.h \
        $(INC)/sys/cmn_err.h \
        $(INC)/sys/kmem.h \
        $(INC)/sys/debug.h \
	$(INC)/fs/fs_subr.h
	${CC} -c ${CFLAGS} ufs_inode.c
 
ufs_inode.L: ufs_inode.c \
        $(INC)/sys/types.h \
        $(INC)/sys/param.h \
        $(INC)/sys/systm.h \
        $(INC)/sys/signal.h \
        $(INC)/sys/cred.h \
        $(INC)/sys/user.h \
        $(INC)/sys/vfs.h \
        $(INC)/sys/stat.h \
        $(INC)/sys/vnode.h \
        $(INC)/sys/buf.h \
        $(INC)/sys/proc.h \
        $(INC)/sys/mode.h \
        $(INC)/sys/cmn_err.h \
        $(INC)/sys/fs/ufs_inode.h \
        $(INC)/sys/fs/ufs_fs.h \
        $(INC)/sys/fs/ufs_quota.h \
        $(INC)/vm/hat.h \
        $(INC)/vm/as.h \
        $(INC)/vm/pvn.h \
        $(INC)/vm/seg.h \
        $(INC)/sys/swap.h \
        $(INC)/sys/sysinfo.h \
        $(INC)/sys/sysmacros.h \
        $(INC)/sys/errno.h \
        $(INC)/sys/cmn_err.h \
        $(INC)/sys/kmem.h \
        $(INC)/sys/debug.h \
	$(INC)/fs/fs_subr.h
	@echo ufs_inode.c:
	@-(${CPP} ${LCOPTS} ufs_inode.c | \
	  ${LINT1} ${LOPTS} > ufs_inode.L ) 2>&1 | ${LTAIL}
 
ufs_subr.o: ufs_subr.c \
        $(INC)/sys/types.h \
        $(INC)/sys/param.h \
	$(INC)/sys/time.h \
        $(INC)/sys/fs/ufs_fs.h \
        $(INC)/sys/systm.h \
        $(INC)/sys/sysmacros.h \
        $(INC)/sys/buf.h \
        $(INC)/sys/conf.h \
        $(INC)/sys/user.h \
        $(INC)/sys/vfs.h \
        $(INC)/sys/vnode.h \
        $(INC)/sys/proc.h \
        $(INC)/sys/debug.h \
        $(INC)/sys/fs/ufs_inode.h \
        $(INC)/vm/hat.h \
        $(INC)/vm/as.h \
        $(INC)/vm/seg.h \
        $(INC)/vm/page.h \
        $(INC)/vm/pvn.h \
        $(INC)/vm/seg_map.h \
        $(INC)/sys/swap.h \
        $(INC)/vm/seg_kmem.h
	${CC} -c ${CFLAGS} ufs_subr.c
 
ufs_subr.L: ufs_subr.c \
        $(INC)/sys/types.h \
        $(INC)/sys/param.h \
	$(INC)/sys/time.h \
        $(INC)/sys/fs/ufs_fs.h \
        $(INC)/sys/systm.h \
        $(INC)/sys/sysmacros.h \
        $(INC)/sys/buf.h \
        $(INC)/sys/conf.h \
        $(INC)/sys/user.h \
        $(INC)/sys/vfs.h \
        $(INC)/sys/vnode.h \
        $(INC)/sys/proc.h \
        $(INC)/sys/debug.h \
        $(INC)/sys/fs/ufs_inode.h \
        $(INC)/vm/hat.h \
        $(INC)/vm/as.h \
        $(INC)/vm/seg.h \
        $(INC)/vm/page.h \
        $(INC)/vm/pvn.h \
        $(INC)/vm/seg_map.h \
        $(INC)/sys/swap.h \
        $(INC)/vm/seg_kmem.h
	@echo ufs_subr.c:
	@-(${CPP} ${LCOPTS} ufs_subr.c | \
	  ${LINT1} ${LOPTS} > ufs_subr.L ) 2>&1 | ${LTAIL}
 
ufs_tables.o: ufs_tables.c \
        $(INC)/sys/types.h \
        $(INC)/sys/param.h
	${CC} -c ${CFLAGS} ufs_tables.c
 
ufs_tables.L: ufs_tables.c \
        $(INC)/sys/types.h \
        $(INC)/sys/param.h
	@echo ufs_tables.c:
	@-(${CPP} ${LCOPTS} ufs_tables.c | \
	  ${LINT1} ${LOPTS} > ufs_tables.L ) 2>&1 | ${LTAIL}
 
ufs_vfsops.o: ufs_vfsops.c \
        $(INC)/sys/types.h \
        $(INC)/sys/param.h \
        $(INC)/sys/systm.h \
        $(INC)/sys/sysmacros.h \
        $(INC)/sys/kmem.h \
        $(INC)/sys/signal.h \
        $(INC)/sys/user.h \
        $(INC)/sys/proc.h \
        $(INC)/sys/buf.h \
        $(INC)/sys/pathname.h \
        $(INC)/sys/vfs.h \
        $(INC)/sys/vnode.h \
        $(INC)/sys/file.h \
        $(INC)/sys/uio.h \
        $(INC)/sys/conf.h \
        $(INC)/sys/fs/ufs_fs.h \
        $(INC)/sys/fs/ufs_inode.h \
        $(INC)/sys/statvfs.h \
        $(INC)/sys/mount.h \
        $(INC)/sys/swap.h \
        $(INC)/sys/errno.h \
	$(INC)/sys/debug.h \
	$(INC)/fs/fs_subr.h
	${CC} -c ${CFLAGS} ufs_vfsops.c
 
ufs_vfsops.L: ufs_vfsops.c \
        $(INC)/sys/types.h \
        $(INC)/sys/param.h \
        $(INC)/sys/systm.h \
        $(INC)/sys/sysmacros.h \
        $(INC)/sys/kmem.h \
        $(INC)/sys/signal.h \
        $(INC)/sys/user.h \
        $(INC)/sys/proc.h \
        $(INC)/sys/buf.h \
        $(INC)/sys/pathname.h \
        $(INC)/sys/vfs.h \
        $(INC)/sys/vnode.h \
        $(INC)/sys/file.h \
        $(INC)/sys/uio.h \
        $(INC)/sys/conf.h \
        $(INC)/sys/fs/ufs_fs.h \
        $(INC)/sys/fs/ufs_inode.h \
        $(INC)/sys/statvfs.h \
        $(INC)/sys/mount.h \
        $(INC)/sys/swap.h \
        $(INC)/sys/errno.h \
	$(INC)/fs/fs_subr.h
	@echo ufs_vfsops.c:
	@-(${CPP} ${LCOPTS} ufs_vfsops.c | \
	  ${LINT1} ${LOPTS} > ufs_vfsops.L ) 2>&1 | ${LTAIL}
 
ufs_vnops.o: ufs_vnops.c \
        $(INC)/sys/types.h \
        $(INC)/sys/param.h \
	$(INC)/sys/time.h \
        $(INC)/sys/systm.h \
        $(INC)/sys/sysmacros.h \
        $(INC)/sys/signal.h \
        $(INC)/sys/cred.h \
        $(INC)/sys/user.h \
        $(INC)/sys/buf.h \
        $(INC)/sys/vfs.h \
        $(INC)/sys/vnode.h \
        $(INC)/sys/proc.h \
        $(INC)/sys/file.h \
        $(INC)/sys/fcntl.h \
        $(INC)/sys/flock.h \
        $(INC)/sys/kmem.h \
        $(INC)/sys/uio.h \
        $(INC)/sys/conf.h \
        $(INC)/sys/mman.h \
        $(INC)/sys/pathname.h \
        $(INC)/sys/debug.h \
        $(INC)/sys/vmmeter.h \
        $(INC)/sys/fs/ufs_fs.h \
        $(INC)/sys/fs/ufs_inode.h \
        $(INC)/sys/fs/ufs_fsdir.h \
        $(INC)/sys/fs/ufs_quota.h \
        $(INC)/sys/dirent.h \
        $(INC)/sys/errno.h \
        $(INC)/sys/sysinfo.h \
        $(INC)/sys/cmn_err.h \
        $(INC)/vm/hat.h \
        $(INC)/vm/page.h \
        $(INC)/vm/pvn.h \
        $(INC)/vm/as.h \
        $(INC)/vm/seg.h \
        $(INC)/vm/seg_map.h \
        $(INC)/vm/seg_vn.h \
        $(INC)/vm/rm.h \
        $(INC)/sys/swap.h \
	$(INC)/fs/fs_subr.h
	${CC} -c ${CFLAGS} ufs_vnops.c
 
ufs_vnops.L: ufs_vnops.c \
        $(INC)/sys/types.h \
        $(INC)/sys/param.h \
	$(INC)/sys/time.h \
        $(INC)/sys/systm.h \
        $(INC)/sys/sysmacros.h \
        $(INC)/sys/signal.h \
        $(INC)/sys/cred.h \
        $(INC)/sys/user.h \
        $(INC)/sys/buf.h \
        $(INC)/sys/vfs.h \
        $(INC)/sys/vnode.h \
        $(INC)/sys/proc.h \
        $(INC)/sys/file.h \
        $(INC)/sys/fcntl.h \
        $(INC)/sys/flock.h \
        $(INC)/sys/kmem.h \
        $(INC)/sys/uio.h \
        $(INC)/sys/conf.h \
        $(INC)/sys/mman.h \
        $(INC)/sys/pathname.h \
        $(INC)/sys/debug.h \
        $(INC)/sys/vmmeter.h \
        $(INC)/sys/fs/ufs_fs.h \
        $(INC)/sys/fs/ufs_inode.h \
        $(INC)/sys/fs/ufs_fsdir.h \
        $(INC)/sys/fs/ufs_quota.h \
        $(INC)/sys/dirent.h \
        $(INC)/sys/errno.h \
        $(INC)/sys/sysinfo.h \
        $(INC)/sys/cmn_err.h \
        $(INC)/vm/hat.h \
        $(INC)/vm/page.h \
        $(INC)/vm/pvn.h \
        $(INC)/vm/as.h \
        $(INC)/vm/seg.h \
        $(INC)/vm/seg_map.h \
        $(INC)/vm/seg_vn.h \
        $(INC)/vm/rm.h \
        $(INC)/sys/swap.h \
	$(INC)/fs/fs_subr.h
	@echo ufs_vnops.c:
	@-(${CPP} ${LCOPTS} ufs_vnops.c | \
	  ${LINT1} ${LOPTS} > ufs_vnops.L ) 2>&1 | ${LTAIL}
 
