#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ufs.cmds:ufs/quota/quota.mk	1.5"

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

quota: quota.c $(OBJS)
	$(CC) -I$(INC) $(CFLAGS) $(LDFLAGS) -o quota quota.c $(OBJS) $(SHLIBS)

install: quota
	if [ ! -d $(DIRDIR) ]; \
	then \
		mkdir $(DIRDIR); \
	fi
	if [ ! -d $(INSDIR) ]; \
	then \
		mkdir $(INSDIR); \
	fi
	$(INS) -f $(INSDIR) -m 0555 -u bin -g bin quota;
	-rm -f $(INSDIR2)/quota
	ln $(INSDIR)/quota $(INSDIR2)/quota
	
clean:
	-rm -f quota.o

clobber: clean
	rm -f quota
