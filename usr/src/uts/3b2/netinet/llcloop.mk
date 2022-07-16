#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)netinet:netinet/llcloop.mk	1.3"
#
#
#  		PROPRIETARY NOTICE (Combined)
#  
#  This source code is unpublished proprietary information
#  constituting, or derived under license from AT&T's Unix(r) System V.
#  In addition, portions of such source code were derived from Berkeley
#  4.3 BSD under license from the Regents of the University of
#  California.
#  
#  
#  
#  		Copyright Notice 
#  
#  Notice of copyright on this source code product does not indicate 
#  publication.
#  
#  	(c) 1986,1987,1988,1989  Sun Microsystems, Inc.
#  	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
#  	          All rights reserved.
#
INC = /usr/include
INSDIR = /boot
MASTERD = ../master.d
MKBOOT = mkboot
PFLAGS = -DSYSV -Du3b2 -D_KERNEL $(MORECPP) -I$(INC)
DASHO = -O
CFLAGS = $(DASHO) $(PFLAGS)
CC = cc
LD = ld
FRC = 

PRODUCTS = LLCLOOP
CLEAN = llcloop.o

.c.o:
	$(CC) $(CFLAGS) -c $*.c

uts: all

all : $(PRODUCTS)

LLCLOOP:	llcloop.o $(MASTERD)/llcloop
	$(MKBOOT) -m $(MASTERD) -d . llcloop.o

llcloop.o:	llcloop.c \
	$(INC)/sys/errno.h \
	$(INC)/sys/fs/s5dir.h \
	$(INC)/sys/lihdr.h \
	$(INC)/netinet/llcloop.h \
	$(INC)/sys/log.h \
	$(INC)/net/if.h \
	$(INC)/net/if_arp.h \
	$(INC)/net/strioc.h \
	$(INC)/netinet/in.h \
	$(INC)/netinet/in_var.h \
	$(INC)/sys/param.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/socket.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/strlog.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/types.h \
	$(INC)/sys/user.h

clean:
	rm -f $(CLEAN)

clobber:	clean
		rm -f $(PRODUCTS)

