#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)emd:io/emd.mk	1.9"
ROOT =
INC = $(ROOT)/usr/include

MACHINE = 3b2

MKBOOT = mkboot
CFLAGS = -I.. -I$(INC) $(MORECPP) -Du$(MACHINE) -D_KERNEL
LDFLAGS = -r
CC = cc
LD = ld

PRODUCTS = emd.o 

CLEAN =	emd.o

.c.o:
	$(CC) $(CFLAGS) -c $*.c
	$(MKBOOT) -m ../master.d -d . $(PRODUCTS)

all : $(PRODUCTS)

emd.o:	emd.c \
	$(INC)/sys/emd.h\
	$(INC)/sys/emduser.h\
	$(INC)/sys/cmn_err.h\
	$(INC)/sys/types.h\
	$(INC)/sys/param.h\
	$(INC)/sys/ddi.h\
	$(INC)/sys/signal.h\
	$(INC)/sys/errno.h\
	$(INC)/sys/psw.h\
	$(INC)/sys/pcb.h\
	$(INC)/sys/user.h\
	$(INC)/sys/stream.h\
	$(INC)/sys/stropts.h\
	$(INC)/sys/strlog.h\
	$(INC)/sys/log.h\
	$(INC)/sys/queue.h\
	$(INC)/sys/cio_defs.h\
	$(INC)/sys/devcode.h\
	$(INC)/sys/firmware.h\
	$(INC)/sys/sbd.h\
	$(INC)/sys/debug.h\
	$(INC)/sys/inline.h\
	$(INC)/sys/systm.h\
	$(INC)/sys/cred.h\
	$(INC)/sys/dlpi.h


clean:
		rm -f $(CLEAN)

clobber:	clean
		rm -f $(PRODUCTS)


