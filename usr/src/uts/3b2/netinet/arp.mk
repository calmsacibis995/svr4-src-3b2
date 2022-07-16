#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)netinet:netinet/arp.mk	1.3"
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


all:	ARP APP

ARP:	arp.o $(MASTERD)/arp
	$(MKBOOT) -m $(MASTERD) -d . arp.o

APP:	app.o $(MASTERD)/app
	$(MKBOOT) -m $(MASTERD) -d . app.o

.c.o:
	$(CC) $(CFLAGS) -c $*.c

arp.o:		arp.c \
	${INC)/sys/errno.h \
	${INC)/sys/inline.h \
	${INC)/sys/lihdr.h \
	${INC)/sys/log.h \
	${INC)/net/if.h \
	${INC)/net/if_arp.h \
	${INC)/net/route.h \
	${INC)/net/strioc.h \
	${INC)/netinet/if_ether.h \
	${INC)/netinet/in.h \
	${INC)/netinet/in_systm.h \
	${INC)/netinet/in_var.h \
	${INC)/netinet/ip.h \
	${INC)/netinet/ip_str.h \
	${INC)/sys/param.h \
	${INC)/sys/socket.h \
	${INC)/sys/stream.h \
	${INC)/sys/strlog.h \
	${INC)/sys/stropts.h \
	${INC)/sys/sysmacros.h \
	${INC)/sys/types.h
