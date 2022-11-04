#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)fs:fs/fifofs/fifofs.mk	1.10"
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
	fifovnops.o \
	fifosubr.o

all:	FIFOFS

FIFOFS:	fifofs.o $(MASTERD)/fifofs
	$(MKBOOT) -m $(MASTERD) -d . fifofs.o

fifofs.o: $(FILES)
	$(LD) -r -o fifofs.o $(FILES)

.c.o:
	$(CC) $(DEFLIST) -I$(INC) $(CFLAGS) -c $*.c

clean:
	-rm -f *.o

clobber:	clean
	-rm -f fifofs FIFOFS

#
# Header dependencies
#

fifosubr.o: fifosubr.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/kmem.h \
	$(INC)/sys/uio.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/user.h \
	$(INC)/sys/fs/fifonode.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/strsubr.h \
	$(INC)/sys/conf.h \
	$(INC)/sys/stropts.h \
	$(INC)/fs/fs_subr.h \
	$(FRC)

fifovnops.o: fifovnops.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/time.h \
	$(INC)/sys/file.h \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/flock.h \
	$(INC)/sys/kmem.h \
	$(INC)/sys/uio.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/user.h \
	$(INC)/sys/fs/fifonode.h \
	$(INC)/sys/conf.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/strsubr.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/proc.h \
	$(INC)/fs/fs_subr.h \
	$(FRC)
