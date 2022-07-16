#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ufs.cmds:ufs/ufsrestore/ufsrestore.mk	1.10"

# CPPFLAGS:
#       DEBUG                   use local directory to find ddate and dumpdates
#       TDEBUG                  trace out the process forking
#
BINS= ufsrestore
OBJS= dirs.o interactive.o main.o restore.o symtab.o \
	tape.o utilities.o
SRCS= dirs.c interactive.c main.c restore.c symtab.c \
	tape.c utilities.c
HDRS= dump.h
ROOT=
INC= $(ROOT)/usr/include
TESTDIR = .
INSDIR1 = $(ROOT)/usr/lib/fs/ufs
INSDIR2 = $(ROOT)/usr/sbin
CFLAGS= -I$(INC)
INS = install
CC= cc
LD= ld
RM= rm -f

all: $(BINS) install clobber

$(BINS): $(OBJS)
	$(CC) $(CFLAGS) -o ufsrestore $(OBJS) $(SHLIBS)

dirs.o:	dirs.c
	$(CC) $(CFLAGS) -c dirs.c

interactive.o:	interactive.c
	$(CC) $(CFLAGS) -c interactive.c

main.o:	main.c
	$(CC) $(CFLAGS) -c main.c

restore.o:	restore.c
	$(CC) $(CFLAGS) -c restore.c

symtab.o:	symtab.c
	$(CC) $(CFLAGS) -c symtab.c

tape.o:	tape.c
	$(CC) $(CFLAGS) -c tape.c

utilities.o:	utilities.c
	$(CC) $(CFLAGS) -c utilities.c

install: $(BINS)
	@if [ ! -d $(ROOT)/usr/lib/fs ]; \
		then \
		mkdir $(ROOT)/usr/lib/fs; \
		fi;
	@if [ ! -d $(INSDIR1) ]; \
		then \
		mkdir $(INSDIR1); \
		fi;
	$(INS) -f $(INSDIR1) -m 0555 -u bin -g bin $(TESTDIR)/ufsrestore
	-rm -f $(INSDIR2)/ufsrestore
	ln $(INSDIR1)/ufsrestore $(INSDIR2)/ufsrestore
	
clean:
	$(RM) $(BINS) $(OBJS)

clobber: clean
	$(RM) ufsrestore
