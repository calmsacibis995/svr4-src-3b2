#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)fs:fs/namefs/namefs.mk	1.4"
ROOT =
STRIP = strip
INC = $(ROOT)/usr/include
MKBOOT = mkboot
MASTERD = ../../master.d
CC = cc

DASHG =
DASHO = -O
PFLAGS = $(DASHG) -D_KERNEL $(MORECPP)
CFLAGS= $(DASHO) $(PFLAGS)
DEFLIST =
FRC =

FILES =\
	namevno.o \
	namevfs.o

all:	NAMEFS

NAMEFS:	namefs.o $(MASTERD)/namefs
	$(MKBOOT) -m $(MASTERD) -d . namefs.o

namefs.o: $(FILES)
	$(LD) -r -o namefs.o $(FILES)

.c.o:
	$(CC) $(DEFLIST) -I$(INC) $(CFLAGS) -c $*.c

clean:
	-rm -f *.o

clobber:	clean
	-rm -f namefs NAMEFS

#
# Header dependencies
#


namevno.o: namevno.c \
	$(INC)/sys/types.h\
	$(INC)/sys/param.h\
	$(INC)/sys/systm.h\
	$(INC)/sys/cred.h\
	$(INC)/sys/errno.h\
	$(INC)/sys/file.h\
	$(INC)/sys/fcntl.h\
	$(INC)/sys/flock.h\
	$(INC)/sys/kmem.h\
	$(INC)/sys/uio.h\
	$(INC)/sys/vfs.h\
	$(INC)/sys/vnode.h\
	$(INC)/sys/immu.h\
	$(INC)/sys/psw.h\
	$(INC)/sys/pcb.h\
	$(INC)/sys/sbd.h\
	$(INC)/sys/signal.h\
	$(INC)/sys/user.h\
	$(INC)/sys/conf.h\
	$(INC)/vm/seg.h\
	$(INC)/sys/fs/namenode.h\
	$(INC)/sys/stream.h\
	$(INC)/fs/fs_subr.h \
	$(FRC)

namevfs.o: namevfs.c \
	$(INC)/sys/types.h\
	$(INC)/sys/param.h\
	$(INC)/sys/debug.h\
	$(INC)/sys/errno.h\
	$(INC)/sys/kmem.h\
	$(INC)/sys/immu.h\
	$(INC)/sys/inline.h\
	$(INC)/sys/file.h\
	$(INC)/sys/proc.h\
	$(INC)/sys/stat.h\
	$(INC)/sys/statvfs.h\
	$(INC)/sys/sysmacros.h\
	$(INC)/sys/var.h\
	$(INC)/sys/vfs.h\
	$(INC)/sys/vnode.h\
	$(INC)/sys/mode.h\
	$(INC)/sys/psw.h\
	$(INC)/sys/pcb.h\
	$(INC)/sys/sbd.h\
	$(INC)/sys/signal.h\
	$(INC)/sys/user.h\
	$(INC)/sys/uio.h\
	$(INC)/sys/cred.h\
	$(INC)/sys/fs/namenode.h\
	$(INC)/sys/stream.h\
	$(INC)/sys/strsubr.h\
	$(INC)/sys/cmn_err.h\
	$(INC)/fs/fs_subr.h \
	$(FRC)



