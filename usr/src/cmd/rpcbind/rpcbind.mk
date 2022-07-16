#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)rpcbind:rpcbind.mk	1.11"

#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#	PROPRIETARY NOTICE (Combined)
#
# This source code is unpublished proprietary information
# constituting, or derived under license from AT&T's UNIX(r) System V.
# In addition, portions of such source code were derived from Berkeley
# 4.3 BSD under license from the Regents of the University of
# California.
#
#
#
#	Copyright Notice 
#
# Notice of copyright on this source code product does not indicate 
#  publication.
#
#	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
#	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
#          All rights reserved.
# 
#
# Makefile for rpcbind
#

LINKLIBS = -lnsl
DESTDIR = $(ROOT)/usr/sbin
INC = $(ROOT)/usr/include
LINT = lint
CPPFLAGS= -DPORTMAP -I$(INC)
CFLAGS = -O $(CPPFLAGS)
INS = install

STRIP = strip

SIZE = size

MAINS = rpcbind

OBJS = rpcbind.o rpcb_svc.o pmap_svc.o check_bound.o stricmp.o ti_opts.o

HDRS = rpcbind.h

SRCS = $(OBJS:.o=.c)

all:	$(OBJS)
	$(CC) $(CFLAGS) -o $(MAINS) $(OBJS) $(LINKLIBS) $(SHLIBS) 

$(OBJS): $(HDRS)

install: all
	$(INS) -f $(DESTDIR) -m 0555 -u bin -g bin $(MAINS)

size: all
	$(SIZE) $(MAINS)

strip: all
	$(STRIP) $(MAINS)

lint:
	$(LINT) $(CFLAGS) $(SRCS) 

clean:
	rm -f $(OBJS)

clobber: clean
	rm -f $(MAINS)
