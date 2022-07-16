#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)rfudaemon:rfudaemon.mk	1.1.6.2"
ROOT =
TESTDIR = .
INSDIR = $(ROOT)/usr/lib/rfs
SYMLINK = :
INC = $(ROOT)/usr/include
INS = install
CFLAGS = -O -s
FRC =

all: rfudaemon

rfudaemon: rfudaemon.c \
	$(INC)/sys/types.h \
	$(INC)/sys/nserve.h \
	$(INC)/sys/rf_sys.h \
	$(INC)/sys/signal.h \
	$(INC)/stdio.h \
	$(INC)/errno.h \
	$(INC)/sys/resource.h
	$(CC) -I$(INC) $(CFLAGS) -o $(TESTDIR)/rfudaemon rfudaemon.c $(SHLIBS)

install: all
	-rm -f $(ROOT)/usr/nserve/rfudaemon
	$(INS) -f $(INSDIR) -m 0550 -u bin -g bin $(TESTDIR)/rfudaemon
	-$(SYMLINK) /usr/lib/rfs/rfudaemon $(ROOT)/usr/nserve/rfudaemon

clean:
	rm -f *.o

clobber: clean
	rm -f $(TESTDIR)/rfudaemon
FRC:
