#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)emd:cmd/emdcmd.mk	1.4"
ROOT =
TESTDIR = .
INSDIR = $(ROOT)/usr/bin
INC = $(ROOT)/usr/include
INCSYS = $(ROOT)/usr/include
MORECPP =
INS = install
CFLAGS = -I$(INC) -I$(INCSYS) $(MORECPP) -O
FRC =

all:	epump emdaddr emdreset emdloop eiasetup

epump:	epump.c\
	$(INC)/stdio.h\
	$(INC)/string.h\
	$(INC)/fcntl.h\
	$(INCSYS)/sys/types.h\
	$(INC)/filehdr.h\
	$(INC)/scnhdr.h\
	$(INC)/ldfcn.h\
	$(INCSYS)/sys/stropts.h\
	$(INCSYS)/sys/emduser.h
	$(CC) $(CFLAGS) -o $(TESTDIR)/epump epump.c -lld $(SHLIBS)

emdaddr:	emdaddr.c\
	$(INC)/stdio.h\
	$(INC)/string.h\
	$(INC)/fcntl.h\
	$(INCSYS)/sys/types.h\
	$(INCSYS)/sys/stropts.h\
	$(INCSYS)/sys/emduser.h
	$(CC) $(CFLAGS) -o $(TESTDIR)/emdaddr emdaddr.c $(SHLIBS)

emdreset:	emdreset.c\
	$(INC)/stdio.h\
	$(INC)/string.h\
	$(INC)/fcntl.h\
	$(INCSYS)/sys/types.h\
	$(INCSYS)/sys/stropts.h\
	$(INCSYS)/sys/emduser.h
	$(CC) $(CFLAGS) -o $(TESTDIR)/emdreset emdreset.c $(SHLIBS)

emdloop:	emdloop.c\
	$(INC)/stdio.h\
	$(INC)/string.h\
	$(INC)/fcntl.h\
	$(INCSYS)/sys/types.h\
	$(INCSYS)/sys/stropts.h\
	$(INCSYS)/sys/emduser.h
	$(CC) $(CFLAGS) -o $(TESTDIR)/emdloop emdloop.c $(SHLIBS)

eiasetup:	eiasetup.c\
	$(INC)/stdio.h\
	$(INC)/fcntl.h\
	$(INCSYS)/sys/firmware.h
	$(CC) $(CFLAGS) -o $(TESTDIR)/eiasetup eiasetup.c $(SHLIBS)

install: all
	$(INS) -f $(INSDIR) -m 0500 -u root -g sys $(TESTDIR)/epump
	$(INS) -f $(INSDIR) -m 0500 -u root -g sys $(TESTDIR)/emdaddr
	$(INS) -f $(INSDIR) -m 0500 -u root -g sys $(TESTDIR)/emdreset
	$(INS) -f $(INSDIR) -m 0500 -u root -g sys $(TESTDIR)/emdloop
	$(INS) -f $(INSDIR) -m 0500 -u root -g sys $(TESTDIR)/eiasetup

clean:
	rm -f *.o

clobber: clean
	rm -f $(TESTDIR)/epump
	rm -f $(TESTDIR)/emdaddr
	rm -f $(TESTDIR)/emdreset
	rm -f $(TESTDIR)/emdloop
	rm -f $(TESTDIR)/eiasetup
FRC:
