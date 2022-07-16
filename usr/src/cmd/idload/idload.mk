#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)idload:idload.mk	1.4.7.1"
ROOT =
TESTDIR = .
INSDIR = $(ROOT)/usr/sbin
INC = $(ROOT)/usr/include
SYMLINK = :
INS = install
CFLAGS = -O -s
LDFLAGS = -lns -lelf $(SHLIBS)
FRC =

all: idload

idload: idload.c \
		$(INC)/stdio.h \
		$(INC)/sys/types.h \
		$(INC)/sys/idtab.h \
		$(INC)/sys/nserve.h \
		$(INC)/sys/rf_sys.h \
		$(INC)/fcntl.h \
		$(INC)/nlist.h \
		$(INC)/nserve.h \
		$(INC)/sys/param.h \
		$(INC)/sys/tiuser.h \
		$(INC)/sys/stat.h \
		$(INC)/nsaddr.h \
		$(INC)/sys/rf_cirmgr.h \
		$(INC)/pn.h
	$(CC) -I$(INC) $(CFLAGS) -o $(TESTDIR)/idload idload.c $(LDFLAGS)

install: all
	-rm -f $(ROOT)/usr/bin/idload
	$(INS) -f $(INSDIR) -m 0555 -u bin -g bin idload
	-$(SYMLINK) /usr/sbin/idload $(ROOT)/usr/bin/idload

clean:
	rm -f *.o

clobber: clean
	rm -f $(TESTDIR)/idload
FRC:
