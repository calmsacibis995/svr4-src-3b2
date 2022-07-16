#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ufs.cmds:ufs/repquota/repquota.mk	1.5"

ROOT=
INC = $(ROOT)/usr/include
INSDIR = $(ROOT)/usr/lib/fs/ufs
DIRDIR = $(ROOT)/usr/lib/fs
INSDIR2 = $(ROOT)/usr/sbin
CFLAGS = -O
LDFLAGS = -s
INS=install
OBJS=

all:  install clobber

repquota: repquota.c $(OBJS)
	$(CC) -I$(INC) $(CFLAGS) $(LDFLAGS) -o repquota repquota.c $(OBJS) $(SHLIBS)

install: repquota
	if [ ! -d $(DIRDIR) ]; \
	then \
		mkdir $(DIRDIR); \
	fi
	if [ ! -d $(INSDIR) ]; \
	then \
		mkdir $(INSDIR); \
	fi
	$(INS) -f $(INSDIR) -m 0555 -u bin -g bin repquota
	-rm -f $(INSDIR2)/repquota
	ln $(INSDIR)/repquota $(INSDIR2)/repquota
	
clean:
	-rm -f repquota.o

clobber: clean
	rm -f repquota
