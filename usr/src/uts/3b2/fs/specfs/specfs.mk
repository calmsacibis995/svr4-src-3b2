#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)fs:fs/specfs/specfs.mk	1.23"
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
	specgetsizes.o \
	specsubr.o \
	specvfsops.o \
	specvnops.o

all:	SPECFS

SPECFS:	specfs.o $(MASTERD)/specfs
	$(MKBOOT) -m $(MASTERD) -d . specfs.o

specfs.o: $(FILES)
	$(LD) -r -o specfs.o $(FILES)

.c.o:
	$(CC) $(DEFLIST) -I$(INC) $(CFLAGS) -c $*.c

clean:
	-rm -f *.o

clobber:	clean
	-rm -f specfs SPECFS

#
# Header dependencies
#

specgetsizes.o: specgetsizes.c \
	$(INC)/sys/types.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/fs/snode.h \
	$(FRC)
	$(CC) $(PFLAGS) -I$(INC) -c -g $<

specsubr.o: specsubr.c \
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
	$(INC)/sys/fs/snode.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/time.h \
	$(INC)/sys/file.h \
        $(INC)/sys/open.h \
        $(INC)/sys/user.h \
	$(FRC)

specvfsops.o: specvfsops.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/swap.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/fs/snode.h \
	$(INC)/fs/fs_subr.h \
	$(FRC)

specvnops.o: specvnops.c \
	$(INC)/sys/session.h \
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
	$(INC)/sys/sysinfo.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/uio.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/poll.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/strsubr.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/user.h \
	$(INC)/sys/fs/snode.h \
	$(INC)/vm/seg.h \
	$(INC)/vm/seg_map.h \
	$(INC)/vm/page.h \
	$(INC)/vm/pvn.h \
	$(INC)/vm/seg_dev.h \
	$(INC)/vm/seg_vn.h \
	$(INC)/fs/fs_subr.h \
	$(FRC)

