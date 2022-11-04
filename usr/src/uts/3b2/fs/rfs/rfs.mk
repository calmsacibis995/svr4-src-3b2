#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)fs:fs/rfs/rfs.mk	1.23"

ROOT =
STRIP = strip
INC = $(ROOT)/usr/include
MKBOOT = mkboot
MASTERD = ../../master.d
DASHG =
DASHO = -O
PFLAGS = $(DASHG) -D_KERNEL $(MORECPP)
CFLAGS= $(DASHO) $(PFLAGS)
DEFLIST=
FRC =

LINT = $(PFX)lint
LINTFLAGS = -unx

MAKEFILE= rfs.mk
MAKEFLAGS =

O = o

FILES = \
	rf_auth.$O \
	rf_cirmgr.$O \
	rf_comm.$O \
	rf_rsrc.$O \
	rf_admin.$O \
	rf_canon.$O \
	rf_sys.$O \
	rfsr_subr.$O \
	rfsr_ops.$O \
	rf_serve.$O \
	du.$O \
	rf_vfsops.$O \
	rf_getsz.$O \
	rf_cache.$O \
	rfcl_subr.$O \
	rf_vnops.$O

.SUFFIXES: .o .ln

.c.$O:
	$(CC) $(DEFLIST) -I$(INC) $(CFLAGS) -c $*.c

all:	RFS

RFS:	rfs.$O $(MASTERD)/rfs
	$(MKBOOT) -m $(MASTERD) -d . rfs.$O

lint:
	$(MAKE) -ef $(MAKEFILE) -$(MAKEFLAGS) CC=$(LINT) INC=$(INC) \
		MORECPP=-UDEBUG O=ln lintit

lintit: $(FILES)
	$(LINT) $(LINTFLAGS) $(FILES)

clean:
	-rm -f *.o  `ls *.ln | egrep -v llib` *.ln

clobber:	clean
	-rm -f RFS llib*

rfs.o:	$(FILES)
	$(LD) -r -o rfs.o $(FILES)

FRC:

#
# Header dependencies
#

rf_auth.$O: \
	$(INC)/sys/types.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/param.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/user.h \
	$(INC)/sys/nserve.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/rf_cirmgr.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/idtab.h \
	$(INC)/sys/inline.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/rf_debug.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/systm.h \
	rf_auth.h \
	$(INC)/sys/kmem.h \
	$(INC)/sys/rf_sys.h \
	rf_canon.h \
	$(FRC)

rf_cirmgr.$O: \
	$(INC)/sys/list.h \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/file.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/user.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/strsubr.h \
	$(INC)/sys/var.h \
	$(INC)/sys/vnode.h \
	$(INC)/vm/seg.h \
	$(INC)/sys/rf_comm.h \
	$(INC)/sys/inline.h \
	rf_admin.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/nserve.h \
	$(INC)/sys/rf_cirmgr.h \
	$(INC)/sys/tihdr.h \
	$(INC)/sys/rf_messg.h \
	$(INC)/sys/hetero.h \
	$(INC)/sys/systm.h \
	rf_auth.h \
	$(INC)/sys/cmn_err.h \
	rf_canon.h \
	$(INC)/sys/strmdep.h \
	$(FRC)

rf_comm.$O: \
	$(INC)/sys/list.h \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/user.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/rf_messg.h \
	$(INC)/vm/seg.h \
	$(INC)/sys/rf_comm.h \
	$(INC)/sys/nserve.h \
	$(INC)/sys/rf_cirmgr.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/rf_debug.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/inline.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/hetero.h \
	rf_canon.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/statfs.h \
	rfcl_subr.h \
	$(INC)/sys/rf_adv.h \
	rf_serve.h \
	$(INC)/sys/uio.h \
	$(INC)/sys/sysinfo.h \
	$(INC)/sys/fs/rf_acct.h \
	$(INC)/sys/fs/rf_vfs.h \
	$(INC)/sys/file.h \
	rf_admin.h \
	rf_cache.h \
	$(INC)/sys/kmem.h \
	$(INC)/sys/fbuf.h \
	$(INC)/vm/page.h \
	$(FRC)

rf_rsrc.$O: \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/nserve.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/list.h \
	$(INC)/sys/rf_adv.h \
	$(INC)/sys/kmem.h \
	$(INC)/sys/rf_sys.h \
	$(FRC)

rf_admin.$O: \
	$(INC)/sys/list.h \
	$(INC)/sys/types.h \
	$(INC)/sys/sysinfo.h \
	$(INC)/sys/time.h \
	$(INC)/sys/fs/rf_acct.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/param.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/siginfo.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/rf_comm.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/user.h \
	$(INC)/sys/rf_messg.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/wait.h \
	$(INC)/sys/nserve.h \
	$(INC)/sys/rf_cirmgr.h \
	$(INC)/sys/rf_debug.h \
	$(INC)/sys/rf_sys.h \
	$(INC)/sys/inline.h \
	$(INC)/sys/rf_adv.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/fs/rf_vfs.h \
	rf_serve.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/statfs.h \
	rfcl_subr.h \
	rf_admin.h \
	$(INC)/sys/uio.h \
	rf_cache.h \
	$(INC)/vm/seg_kmem.h \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/flock.h \
	$(INC)/sys/file.h \
	$(INC)/sys/session.h \
	$(FRC)

rf_canon.$O: \
	$(INC)/sys/types.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/param.h \
	$(INC)/sys/rf_messg.h \
	$(INC)/sys/nserve.h \
	$(INC)/sys/rf_cirmgr.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/dirent.h \
	rf_canon.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/errno.h \
	$(FRC)

rf_sys.$O: \
	$(INC)/sys/list.h \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/nserve.h \
	$(INC)/sys/rf_cirmgr.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/user.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/vfs.h \
	$(INC)/vm/seg.h \
	$(INC)/sys/rf_comm.h \
	$(INC)/sys/rf_messg.h \
	$(INC)/sys/rf_debug.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/rf_adv.h \
	$(INC)/sys/rf_sys.h \
	$(INC)/sys/inline.h \
	$(INC)/sys/idtab.h \
	$(INC)/sys/hetero.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/fs/rf_vfs.h \
	$(INC)/sys/var.h \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/uio.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/statvfs.h \
	$(INC)/sys/statfs.h \
	rf_auth.h \
	$(INC)/sys/dirent.h \
	rf_admin.h \
	$(INC)/sys/cred.h \
	rf_cache.h \
	$(INC)/sys/file.h \
	rfcl_subr.h \
	$(INC)/sys/pathname.h \
	$(INC)/sys/kmem.h \
	rf_serve.h \
	$(FRC)

rfsr_subr.$O: \
	$(INC)/sys/list.h \
	$(INC)/sys/types.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/param.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/mode.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/stream.h \
	$(INC)/vm/seg.h \
	$(INC)/sys/rf_comm.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/user.h \
	$(INC)/sys/siginfo.h \
	$(INC)/sys/nserve.h \
	$(INC)/sys/rf_cirmgr.h \
	$(INC)/sys/idtab.h \
	$(INC)/sys/var.h \
	$(INC)/sys/file.h \
	$(INC)/sys/pathname.h \
	$(INC)/sys/fstyp.h \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/inline.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/conf.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/rf_adv.h \
	$(INC)/sys/uio.h \
	$(INC)/sys/fs/rf_vfs.h \
	$(INC)/sys/rf_messg.h \
	rf_serve.h \
	$(INC)/sys/statfs.h \
	$(INC)/sys/statvfs.h \
	rf_auth.h \
	rfcl_subr.h \
	rf_cache.h \
	$(INC)/sys/hetero.h \
	rf_canon.h \
	$(INC)/sys/sysinfo.h \
	$(INC)/sys/fs/rf_acct.h \
	$(FRC)

rfsr_ops.$O: \
	$(INC)/sys/list.h \
	$(INC)/sys/types.h \
	$(INC)/sys/sysinfo.h \
	$(INC)/sys/time.h \
	$(INC)/sys/fs/rf_acct.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/param.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/mode.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/stream.h \
	$(INC)/vm/seg.h \
	$(INC)/sys/rf_comm.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/user.h \
	$(INC)/sys/dirent.h \
	$(INC)/sys/nserve.h \
	$(INC)/sys/rf_cirmgr.h \
	$(INC)/sys/idtab.h \
	$(INC)/sys/rf_messg.h \
	$(INC)/sys/var.h \
	$(INC)/sys/file.h \
	$(INC)/sys/fstyp.h \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/statfs.h \
	$(INC)/sys/inline.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/rf_debug.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/conf.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/rf_adv.h \
	$(INC)/sys/uio.h \
	$(INC)/sys/fs/rf_vfs.h \
	$(INC)/sys/pathname.h \
	$(INC)/sys/hetero.h \
	rf_serve.h \
	$(INC)/sys/ustat.h \
	$(INC)/sys/statvfs.h \
	rfcl_subr.h \
	rf_auth.h \
	$(INC)/sys/fbuf.h \
	$(INC)/vm/seg_map.h \
	rf_canon.h \
	$(INC)/sys/mman.h \
	rf_cache.h \
	du.h \
	$(INC)/sys/kmem.h \
	$(FRC)

rf_serve.$O: \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/time.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/nserve.h \
	$(INC)/sys/rf_cirmgr.h \
	$(INC)/sys/list.h \
	$(INC)/vm/seg.h \
	$(INC)/sys/rf_comm.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/user.h \
	$(INC)/sys/rf_messg.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/inline.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/idtab.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/mode.h \
	$(INC)/sys/rf_adv.h \
	rf_serve.h \
	$(INC)/sys/file.h \
	$(INC)/sys/acct.h \
	$(INC)/sys/systm.h \
	rf_auth.h \
	rf_admin.h \
	$(FRC)

du.$O: \
	$(INC)/sys/types.h \
	$(INC)/sys/sysinfo.h \
	$(INC)/sys/fs/rf_acct.h \
	$(INC)/sys/param.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/user.h \
	$(INC)/sys/uio.h \
	$(INC)/sys/file.h \
	$(INC)/sys/pathname.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/conf.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/rf_messg.h \
	$(INC)/sys/nserve.h \
	$(INC)/sys/list.h \
	$(INC)/sys/mode.h \
	$(INC)/sys/idtab.h \
	$(INC)/sys/rf_cirmgr.h \
	$(INC)/vm/seg.h \
	$(INC)/sys/rf_comm.h \
	$(INC)/sys/fs/rf_vfs.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/rf_debug.h \
	$(INC)/sys/rf_adv.h \
	rf_serve.h \
	$(INC)/sys/kmem.h \
	$(INC)/sys/utime.h \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/statfs.h \
	$(INC)/sys/statvfs.h \
	$(INC)/sys/hetero.h \
	rf_canon.h \
	rfcl_subr.h \
	rf_auth.h \
	du.h \
	$(INC)/sys/inline.h \
	rf_cache.h \
	$(FRC)

rf_vfsops.$O: \
	$(INC)/sys/list.h \
	$(INC)/sys/types.h \
	$(INC)/sys/sysinfo.h \
	$(INC)/sys/time.h \
	$(INC)/sys/fs/rf_acct.h \
	$(INC)/sys/bitmap.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/param.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/user.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/pathname.h \
	$(INC)/vm/seg.h \
	$(INC)/sys/rf_comm.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/rf_messg.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/rf_debug.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/nserve.h \
	$(INC)/sys/rf_cirmgr.h \
	$(INC)/sys/fs/rf_vfs.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/statfs.h \
	$(INC)/sys/statvfs.h \
	$(INC)/sys/inline.h \
	$(INC)/sys/rf_adv.h \
	rfcl_subr.h \
	du.h \
	$(INC)/sys/mount.h \
	$(INC)/sys/mode.h \
	$(INC)/sys/hetero.h \
	rf_canon.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/uio.h \
	rf_cache.h \
	$(INC)/sys/kmem.h \
	$(FRC)

rf_getsz.$O: \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/rf_adv.h \
	$(INC)/sys/nserve.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/statfs.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/rf_messg.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/pathname.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/list.h \
	$(INC)/sys/rf_cirmgr.h \
	$(INC)/sys/rf_debug.h \
	$(INC)/vm/seg.h \
	$(INC)/sys/rf_comm.h \
	rf_admin.h \
	rf_serve.h \
	$(INC)/sys/fs/rf_vfs.h \
	rfcl_subr.h \
	du.h \
	$(FRC)
	$(CC) $(PFLAGS) -I$(INC) -c -g $<

rf_cache.$O: \
	$(INC)/sys/list.h \
	$(INC)/sys/types.h \
	$(INC)/sys/sysinfo.h \
	$(INC)/sys/fs/rf_acct.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/param.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/rf_messg.h \
	$(INC)/sys/uio.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/nserve.h \
	$(INC)/sys/rf_cirmgr.h \
	$(INC)/sys/hetero.h \
	$(INC)/vm/seg_map.h \
	$(INC)/sys/rf_comm.h \
	$(INC)/vm/seg.h \
	$(INC)/vm/page.h \
	$(INC)/vm/pvn.h \
	$(INC)/vm/seg_vn.h \
	$(INC)/vm/rm.h \
	$(INC)/sys/mman.h \
	$(INC)/sys/file.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/statfs.h \
	rf_cache.h \
	rfcl_subr.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/user.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/kmem.h \
	$(INC)/sys/fs/rf_vfs.h \
	$(FRC)

rfcl_subr.$O: \
	$(INC)/sys/list.h \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/rf_messg.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/nserve.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/rf_cirmgr.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/fs/rf_vfs.h \
	$(INC)/sys/fs/rf_acct.h \
	$(INC)/sys/rf_sys.h \
	$(INC)/vm/seg.h \
	$(INC)/sys/rf_comm.h \
	$(INC)/sys/file.h \
	$(INC)/sys/inline.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/hetero.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/user.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/statfs.h \
	rfcl_subr.h \
	$(INC)/sys/uio.h \
	$(INC)/sys/kmem.h \
	$(INC)/sys/mount.h \
	$(INC)/sys/systm.h \
	rf_cache.h \
	$(INC)/sys/mode.h \
	$(INC)/sys/pathname.h \
	$(INC)/vm/page.h \
	$(INC)/vm/seg_map.h \
	$(INC)/vm/pvn.h \
	$(FRC)

rf_vnops.$O: \
	$(INC)/sys/list.h \
	$(INC)/sys/types.h \
	$(INC)/sys/sysinfo.h \
	$(INC)/sys/fs/rf_acct.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/param.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/uio.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/user.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/statvfs.h \
	$(INC)/vm/seg.h \
	$(INC)/sys/rf_comm.h \
	$(INC)/sys/nserve.h \
	$(INC)/sys/rf_cirmgr.h \
	$(INC)/sys/rf_messg.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/statfs.h \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/file.h \
	$(INC)/sys/pathname.h \
	$(INC)/sys/mode.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/mman.h \
	$(INC)/vm/page.h \
	$(INC)/vm/pvn.h \
	$(INC)/vm/rm.h \
	$(INC)/vm/seg_vn.h \
	$(INC)/sys/fs/rf_vfs.h \
	$(INC)/fs/fs_subr.h \
	$(INC)/sys/hetero.h \
	rf_canon.h \
	rfcl_subr.h \
	du.h \
	rf_cache.h \
	rf_auth.h \
	$(INC)/sys/idtab.h \
	$(INC)/sys/inline.h \
	$(INC)/sys/kmem.h \
	$(INC)/vm/seg_map.h \
	$(FRC)
