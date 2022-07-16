#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)bbh:bbh.mk	1.7.4.1"
ROOT =
INC = $(ROOT)/usr/include
CFLAGS = -O -I$(INC) -D_STYPES
LDFLAGS = -s
SYMLINK = :
MAKE = make "AS=$(AS)" "CC=$(CC)" "LD=$(LD)"
INS = install

all:	hdeadd hdefix hdelogger

hdeadd:	dconv.o edio.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o hdeadd hdeadd.c dconv.o edio.o $(SHLIBS)

hdefix:	edio.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o hdefix hdefix.c edio.o $(ROOTLIBS)

hdelogger: edio.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o hdelogger hdelogger.c edio.o $(SHLIBS)

install: all
	-rm -f $(ROOT)/etc/hdeadd
	-rm -f $(ROOT)/etc/hdefix
	-rm -f $(ROOT)/etc/hdelogger
	-rm -f $(ROOT)/usr/sbin/hdefix
	$(INS) -f $(ROOT)/usr/sbin -m 0555 -u bin -g bin hdeadd
	$(INS) -o -f $(ROOT)/usr/sbin -m 0555 -u bin -g bin hdelogger
	$(INS) -f $(ROOT)/sbin -m 0555 -u bin -g bin hdefix
	$(INS) -f $(ROOT)/usr/sbin -m 0555 -u bin -g bin hdefix
	$(SYMLINK) /sbin/hdefix $(ROOT)/etc/hdefix
	$(SYMLINK) /usr/sbin/hdeadd $(ROOT)/etc/hdeadd
	$(SYMLINK) /usr/sbin/hdelogger $(ROOT)/etc/hdelogger

clean:
	-rm -f hdeadd.o hdefix.o hdelogger.o edio.o dconv.o

clobber: clean
	-rm -f hdeadd hdefix hdelogger

FRC:

#
# Header dependencies
#

hdeadd: hdeadd.c \
	$(INC)/fcntl.h \
	$(INC)/stdio.h \
	$(INC)/sys/hdeioctl.h \
	$(INC)/sys/hdelog.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/types.h \
	$(INC)/sys/vtoc.h \
	dconv.h \
	edio.h \
	hdecmds.h \
	$(FRC)

hdefix: hdefix.c \
	$(INC)/fcntl.h \
	$(INC)/stdio.h \
	$(INC)/sys/filsys.h \
	$(INC)/sys/hdeioctl.h \
	$(INC)/sys/hdelog.h \
	$(INC)/sys/param.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/types.h \
	$(INC)/sys/uadmin.h \
	$(INC)/sys/vtoc.h \
	$(INC)/utmp.h \
	edio.h \
	hdecmds.h \
	$(FRC)

hdelogger: hdelogger.c \
	$(INC)/stdio.h \
	$(INC)/sys/hdeioctl.h \
	$(INC)/sys/hdelog.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/types.h \
	$(INC)/sys/vtoc.h \
	$(INC)/time.h \
	edio.h \
	hdecmds.h \
	$(FRC)

edio.o: edio.c \
	$(INC)/errno.h \
	$(INC)/fcntl.h \
	$(INC)/stdio.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/hdeioctl.h \
	$(INC)/sys/hdelog.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/types.h \
	$(INC)/sys/vtoc.h \
	edio.h \
	hdecmds.h \
	$(FRC)

dconv.o: dconv.c \
	$(INC)/time.h \
	$(FRC)
