#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ufs.cmds:ufs/ncheck/ncheck.mk	1.3"

ROOT=
INC = $(ROOT)/usr/include
INSDIR = $(ROOT)/usr/lib/fs/ufs
DIRDIR = $(ROOT)/usr/lib/fs
CFLAGS = -O
LDFLAGS = -s 
INS=install
OBJS=

all:  install clobber

ncheck: ncheck.c $(OBJS)
	$(CC) -I$(INC) $(CFLAGS) $(LDFLAGS) -o ncheck ncheck.c $(OBJS) $(SHLIBS)

install: ncheck
	if [ ! -d $(DIRDIR) ]; \
	then \
		mkdir $(DIRDIR); \
	fi
	if [ ! -d $(INSDIR) ]; \
	then \
		mkdir $(INSDIR); \
	fi
	$(INS) -f $(INSDIR) -m 0555 -u bin -g bin ncheck

clean:
	-rm -f ncheck.o

clobber: clean
	rm -f ncheck
