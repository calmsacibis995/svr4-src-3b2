#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ufs.cmds:ufs/volcopy/volcopy.mk	1.5"

ROOT=
TESTDIR = .
INC = $(ROOT)/usr/include
INSDIR = $(ROOT)/usr/lib/fs/ufs
DIRDIR = $(ROOT)/usr/lib/fs
CFLAGS = -g
LDFLAGS = 
INS=install
OBJS=

all:  install clobber

volcopy: volcopy.c $(OBJS)
	$(CC) -I$(INC) $(CFLAGS) $(LDFLAGS) -o volcopy volcopy.c $(OBJS) -lgenIO $(SHLIBS)

install: volcopy
	@if [ ! -d $(DIRDIR) ]; \
	then \
		mkdir $(DIRDIR); \
	fi
	@if [ ! -d $(INSDIR) ]; \
	then \
		mkdir $(INSDIR); \
 	fi
	$(INS) -f $(INSDIR) -m 0555 -u bin -g bin $(TESTDIR)/volcopy

clean:
	-rm -f volcopy.o

clobber: clean
	rm -f volcopy
