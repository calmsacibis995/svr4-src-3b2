#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kernel:io/xt.mk	1.10"
#
# STREAMS XT
#
ROOT = 
STRIP = strip
DBO = -DDBO
MKBOOT = mkboot
MASTERD = ../master.d
INC = $(ROOT)/usr/include
PFLAGS = -I$(INC) -D_KERNEL -DSVR40 $(MORECPP) $(DBO)
CFLAGS = $(DASHO) $(PFLAGS)
FRC =

DFILES = \
	xt.o

drivers:	all

all:	XT

clean:
	-rm -f xt.o

clobber:	clean
	-rm -f XT

install:	drivers

FRC:

XT:	$(DFILES) $(MASTERD)/xt
	$(MKBOOT) -m $(MASTERD) -d . xt.o

boot:
	mkboot -m ../master.d -d . xt.o

#
# Header dependencies
#

xt.o: \
	$(INC)/sys/types.h \
	$(INC)/sys/inline.h \
	$(INC)/sys/param.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/conf.h \
	$(INC)/sys/signal.h \
 	$(INC)/sys/dir.h \
 	$(INC)/sys/proc.h \
 	$(INC)/sys/psw.h \
 	$(INC)/sys/pcb.h \
 	$(INC)/sys/user.h \
 	$(INC)/sys/errno.h \
 	$(INC)/sys/debug.h \
 	$(INC)/sys/jioctl.h \
 	$(INC)/sys/termio.h \
 	$(INC)/sys/stream.h \
 	$(INC)/sys/stropts.h \
 	$(INC)/sys/nxtproto.h \
 	$(INC)/sys/nxt.h \
 	$(INC)/sys/eucioctl.h \
 	$(INC)/sys/fcntl.h \
 	$(INC)/sys/tty.h \
 	$(INC)/sys/cmn_err.h \
	$(FRC)

