#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kernel:io/pipemod.mk	1.2"
ROOT =
STRIP = strip
INC = $(ROOT)/usr/src/uts/3b2
MKBOOT = mkboot
MASTERD = ../master.d

DASHO = -O 
PFLAGS= -I$(INC) -D_KERNEL $(MORECPP)
CFLAGS= $(DASHO) -Uvax -Updp11 -Uu3b -Uu3b15 -Du3b2 -UDEBUG $(PFLAGS)
FRC =

DFILES =pipemod.o
drivers:	all

all:	PIPEMOD 
PIPEMOD:	pipemod.o $(MASTERD)/pipemod
	$(MKBOOT) -m $(MASTERD) -d . pipemod.o;

clean:
	-rm -f $(DFILES)

clobber:	clean
	-rm -f PIPEMOD

FRC:

#
# Header dependencies
#

pipemod.o: pipemod.c \
	$(INC)/sys/types.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/param.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/user.h \
	$(INC)/sys/fstyp.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/file.h \
	$(INC)/sys/fs/fifonode.h \
	$(FRC)

