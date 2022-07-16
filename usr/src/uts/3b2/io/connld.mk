#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kernel:io/connld.mk	1.2"
ROOT =
STRIP = strip
INC = $(ROOT)/usr/src/uts/3b2
MKBOOT = mkboot
MASTERD = ../master.d

DASHO = -O 
PFLAGS= -I$(INC) -D_KERNEL $(MORECPP)
CFLAGS= $(DASHO) -Uvax -Updp11 -Uu3b -Uu3b15 -Du3b2 -UDEBUG $(PFLAGS)
FRC =

DFILES =connld.o
drivers:	all

all:	CONNLD 
CONNLD:	connld.o $(MASTERD)/connld
	$(MKBOOT) -m $(MASTERD) -d . connld.o;

clean:
	-rm -f $(DFILES)

clobber:	clean
	-rm -f CONNLD

FRC:

#
# Header dependencies
#

connld.o: connld.c \
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
	$(INC)/sys/strsubr.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/file.h \
	$(INC)/sys/fs/fifonode.h \
	$(FRC)

