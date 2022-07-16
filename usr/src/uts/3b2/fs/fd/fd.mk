#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)fs:fs/fd/fd.mk	1.1"

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

FILES = fdops.o
all:	FD

FD:	fd.o $(MASTERD)/fd
	$(MKBOOT) -m $(MASTERD) -d . fd.o

fd.o: $(FILES)
	$(LD) -r -o fd.o $(FILES)

.c.o:
	$(CC) $(DEFLIST) -I$(INC) $(CFLAGS) -c $*.c

lint:
	lint -u -x $(DEFLIST) -I$(INC) $(CFLAGS) \
		fdops.c

clean:
	-rm -f *.o

clobber:	clean
	-rm -f FD

#
# Header dependencies
#
fdops.o: fdops.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/dirent.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/file.h \
	$(INC)/sys/inline.h \
	$(INC)/sys/kmem.h \
	$(INC)/sys/pathname.h \
	$(INC)/sys/statvfs.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/uio.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/user.h \
	$(INC)/fs/fs_subr.h \
	$(FRC)

