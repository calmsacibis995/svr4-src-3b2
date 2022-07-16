#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kernel:io/ttcompat.mk	1.3"

ROOT =
STRIP = strip
INC = $(ROOT)/usr/include
MKBOOT = mkboot
MASTERD = ../master.d

DASHO = -O 
PFLAGS= -I$(INC) -D_KERNEL $(MORECPP)
CFLAGS= $(DASHO) -Uvax -Updp11 -Uu3b -Uu3b15 -Du3b2 $(PFLAGS)
FRC =

DFILES =ttcompat.o 
drivers:	all

all:	TTCOMPAT 
TTCOMPAT:	ttcompat.o $(MASTERD)/ttcompat
	$(MKBOOT) -m $(MASTERD) -d . ttcompat.o

clean:
	-rm -f $(DFILES)

clobber:	clean

FRC:

#
# Header dependencies
#

ttcompat.o: ttcompat.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/file.h \
	$(INC)/sys/user.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/termios.h \
	$(INC)/sys/ttold.h \
	$(INC)/sys/cmn_err.h\
	$(INC)/sys/stream.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/ttcompat.h \
	$(FRC)
