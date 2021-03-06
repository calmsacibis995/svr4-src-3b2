#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)rpcinfo:rpcinfo.mk	1.10"

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
# Makefile for rpcinfo
#

LINKLIBS= -lnsl
LINT	= lint
DESTDIR = $(ROOT)/usr/bin
INC	= $(ROOT)/usr/include
CPPFLAGS= -O -DPORTMAP -I$(INC)
CFLAGS	= $(CPPFLAGS)
INS = install

GOAL = rpcinfo

OBJS = rpcinfo.o
SRCS = $(OBJS:.o=.c)

$(GOAL): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LINKLIBS) $(LDFLAGS)  $(SHLIBS) 

install: $(GOAL)
	$(INS) -f $(DESTDIR) -m 0555 -u bin -g bin $(GOAL)

lint:
	$(LINT) $(CPPFLAGS) $(SRCS) 

clean:
	rm -f $(OBJS)

clobber: clean
	rm -f $(GOAL)
