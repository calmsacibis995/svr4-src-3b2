#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)fs:fs/nfs/nfs.mk	1.6"
#
#
#  		PROPRIETARY NOTICE (Combined)
#  
#  This source code is unpublished proprietary information
#  constituting, or derived under license from AT&T's Unix(r) System V.
#  
#  
#  
#  		Copyright Notice 
#  
#  Notice of copyright on this source code product does not indicate 
#  publication.
#  
#  	(c) 1986,1987,1988,1989  Sun Microsystems, Inc.
#  	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
#  	          All rights reserved.
#
ROOT =
STRIP = strip
INC = $(ROOT)/usr/include
MKBOOT = mkboot
MASTERD = ../../master.d

DASHG =
DASHO = -O
PFLAGS = $(DASHG) -D_KERNEL -DSYSV $(MORECPP)
CFLAGS = $(DASHO) $(PFLAGS)
DEFLIST =
FRC =

FILES = \
	nfs_aux.o\
	nfs_client.o\
	nfs_common.o\
	nfs_export.o\
	nfs_server.o\
	nfs_subr.o\
	nfs_vfsops.o\
	nfs_vnops.o\
	nfs_xdr.o\
	nfssys.o

#	nfs_dump.o\

all:	NFS

NFS:	nfs.o $(MASTERD)/nfs
	$(MKBOOT) -m $(MASTERD) -d . nfs.o

nfs.o: $(FILES)
	$(LD) -r -o nfs.o $(FILES)

.c.o:
	$(CC) $(DEFLIST) -I$(INC) $(CFLAGS) -c $*.c

clean:
	-rm -f *.o

clobber:	clean
	-rm -f NFS

#
# Header dependencies
#

nfs_aux.o: nfs_aux.c \
	$(INC)/sys/types.h \
	$(INC)/sys/vfs.h \
	$(FRC)

nfs_client.o: nfs_client.c \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/time.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/kmem.h \
	$(INC)/vm/pvn.h \
	$(INC)/rpc/types.h \
	$(INC)/rpc/xdr.h \
	$(INC)/rpc/auth.h \
	$(INC)/rpc/clnt.h \
	$(INC)/sys/tiuser.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/nfs/nfs.h \
	$(INC)/nfs/nfs_clnt.h \
	$(INC)/nfs/rnode.h \
	$(FRC)

nfs_common.o: nfs_common.c \
	$(INC)/sys/errno.h \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/user.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/time.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/vnode.h \
	$(INC)/rpc/types.h \
	$(INC)/nfs/nfs.h \
	$(INC)/sys/mode.h \
	$(FRC)

nfs_export.o: nfs_export.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/time.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/socket.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/uio.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/user.h \
	$(INC)/sys/file.h \
	$(INC)/sys/kmem.h \
	$(INC)/sys/pathname.h \
	$(INC)/netinet/in.h \
	$(INC)/rpc/types.h \
	$(INC)/rpc/auth.h \
	$(INC)/rpc/auth_unix.h \
	$(INC)/rpc/auth_des.h \
	$(INC)/nfs/nfs.h \
	$(INC)/nfs/export.h \
	$(FRC)

nfs_server.o: nfs_server.c \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/user.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/pathname.h \
	$(INC)/sys/uio.h \
	$(INC)/sys/file.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/socket.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/siginfo.h \
	$(INC)/netinet/in.h \
	$(INC)/sys/tiuser.h \
	$(INC)/sys/statvfs.h \
	$(INC)/sys/t_kuser.h \
	$(INC)/sys/kmem.h \
	$(INC)/rpc/types.h \
	$(INC)/rpc/auth.h \
	$(INC)/rpc/auth_unix.h \
	$(INC)/rpc/auth_des.h \
	$(INC)/rpc/svc.h \
	$(INC)/rpc/xdr.h \
	$(INC)/nfs/nfs.h \
	$(INC)/nfs/export.h \
	$(INC)/sys/dirent.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/debug.h \
	$(INC)/vm/hat.h \
	$(INC)/vm/as.h \
	$(INC)/vm/seg.h \
	$(INC)/vm/seg_map.h \
	$(INC)/vm/seg_kmem.h \
	$(FRC)

nfs_subr.o: nfs_subr.c \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/user.h \
	$(INC)/sys/time.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/socket.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/uio.h \
	$(INC)/sys/tiuser.h \
	$(INC)/sys/swap.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/kmem.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/netinet/in.h \
	$(INC)/rpc/types.h \
	$(INC)/rpc/xdr.h \
	$(INC)/rpc/auth.h \
	$(INC)/rpc/clnt.h \
	$(INC)/nfs/nfs.h \
	$(INC)/nfs/nfs_clnt.h \
	$(INC)/nfs/rnode.h \
	$(INC)/vm/pvn.h \
	$(FRC)

nfs_vfsops.o: nfs_vfsops.c \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/user.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/pathname.h \
	$(INC)/sys/uio.h \
	$(INC)/sys/socket.h \
	$(INC)/sys/tiuser.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/kmem.h \
	$(INC)/netinet/in.h \
	$(INC)/rpc/types.h \
	$(INC)/rpc/xdr.h \
	$(INC)/rpc/auth.h \
	$(INC)/rpc/clnt.h \
	$(INC)/rpc/pmap_prot.h \
	$(INC)/nfs/nfs.h \
	$(INC)/nfs/nfs_clnt.h \
	$(INC)/nfs/rnode.h \
	$(INC)/nfs/mount.h \
	$(INC)/sys/mount.h \
	$(INC)/sys/ioctl.h \
	$(INC)/sys/statvfs.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/debug.h \
	$(FRC)

nfs_vnops.o: nfs_vnops.c \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/user.h \
	$(INC)/sys/time.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/file.h \
	$(INC)/sys/uio.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/mman.h \
	$(INC)/sys/tiuser.h \
	$(INC)/sys/pathname.h \
	$(INC)/sys/dirent.h \
	$(INC)/sys/conf.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/vmmeter.h \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/flock.h \
	$(INC)/sys/swap.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/disp.h \
	$(INC)/sys/kmem.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/rpc/types.h \
	$(INC)/rpc/auth.h \
	$(INC)/rpc/clnt.h \
	$(INC)/rpc/xdr.h \
	$(INC)/nfs/nfs.h \
	$(INC)/nfs/nfs_clnt.h \
	$(INC)/nfs/rnode.h \
	$(INC)/vm/hat.h \
	$(INC)/vm/as.h \
	$(INC)/vm/page.h \
	$(INC)/vm/pvn.h \
	$(INC)/vm/seg.h \
	$(INC)/vm/seg_map.h \
	$(INC)/vm/seg_vn.h \
	$(INC)/vm/rm.h \
	$(INC)/fs/fs_subr.h \
	$(INC)/klm/lockmgr.h \
	$(FRC)

nfs_xdr.o: nfs_xdr.c \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/user.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/file.h \
	$(INC)/sys/dirent.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/stream.h \
	$(INC)/rpc/types.h \
	$(INC)/rpc/xdr.h \
	$(INC)/nfs/nfs.h \
	$(INC)/netinet/in.h \
	$(INC)/vm/hat.h \
	$(INC)/vm/as.h \
	$(INC)/vm/seg.h \
	$(INC)/vm/seg_map.h \
	$(INC)/vm/seg_kmem.h \
	$(FRC)

nfssys.o: nfssys.c \
	$(INC)/sys/types.h \
	$(INC)/rpc/types.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/errno.h \
	$(INC)/nfs/nfs.h \
	$(INC)/nfs/export.h \
	$(INC)/nfs/nfssys.h \
	$(FRC)
