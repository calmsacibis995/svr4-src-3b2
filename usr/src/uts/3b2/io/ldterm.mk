#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kernel:io/ldterm.mk	1.9"
ROOT =
STRIP = strip
INC = $(ROOT)/usr/include
MKBOOT = mkboot
MASTERD = ../master.d

DASHO = -O 
PFLAGS = -I$(INC) -D_KERNEL $(MORECPP)
CFLAGS = $(DASHO) -DDBUG -DSVR40 $(PFLAGS)
FRC =

DFILES = ldterm.o 

drivers:	all

all:	LDTERM 

clean:
	-rm -f $(DFILES)

clobber:	clean
	-rm -f LDTERM

FRC:

LDTERM:	ldterm.o $(MASTERD)/ldterm
	$(MKBOOT) -m $(MASTERD) -d . ldterm.o

#
# Header dependencies
#

ldterm.o: ldterm.c \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/termios.h \
	$(INC)/sys/termio.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/conf.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/strtty.h \
	$(INC)/sys/tty.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/dir.h \
	$(INC)/sys/user.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/euc.h \
	$(INC)/sys/eucioctl.h \
	$(INC)/sys/ldterm.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/cred.h \
	$(FRC)
