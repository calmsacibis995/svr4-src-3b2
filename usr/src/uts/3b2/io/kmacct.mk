#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kernel:io/kmacct.mk	1.1"

ROOT	= 
STRIP	= strip
INC	= $(ROOT)/usr/include
MKBOOT	= mkboot
MASTERD	= ../master.d

DASHO	= -O
PFLAGS	= -I$(INC) -D_KERNEL $(MORECPP)
CFLAGS	= $(DASHO) $(PFLAGS)
FRC	=

DFILES	= kmacct.o


all:		KMACCT

clean:
		-rm -f $(DFILES)

clobber:	clean
		-rm -f KMACCT

FRC:

KMACCT:		$(DFILES) $(MASTERD)/kmacct
		$(MKBOOT) -m $(MASTERD) -d . kmacct.o

#
# Header dependencies
#

kmacct.o:	kmacct.c \
		$(INC)/sys/types.h \
		$(INC)/sys/param.h \
		$(INC)/sys/vnode.h \
		$(INC)/sys/psw.h \
		$(INC)/sys/pcb.h \
		$(INC)/sys/signal.h \
		$(INC)/sys/dir.h \
		$(INC)/sys/file.h \
		$(INC)/sys/user.h \
		$(INC)/sys/cmn_err.h \
		$(INC)/sys/errno.h \
		$(INC)/sys/debug.h \
		$(INC)/sys/kmacct.h \
		$(FRC)
