#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)drvinstal:drvinstal.mk	1.10.5.2"

ROOT =
INC = $(ROOT)/usr/include
SYMLINK = :
CFLAGS =-O -I$(INC)
LDFLAGS=-s
INS = install
FRC =

all: drvinstall 

install: all
	-rm -f $(ROOT)/etc/drvinstall
	$(INS) -f $(ROOT)/usr/sbin -m 0555 -u bin -g bin drvinstall
	-$(SYMLINK) /usr/sbin/drvinstall $(ROOT)/etc/drvinstall

drvinstall:
	$(CC) $(CFLAGS) -o drvinstall drvinstal.c $(LDFLAGS) $(NOSHLIBS)

clean:

clobber:	clean
		rm -f drvinstall 

FRC:

#
# Header dependencies
#

drvinstall: drvinstal.c \
	$(INC)/ctype.h \
	$(INC)/dirent.h \
	$(INC)/fcntl.h \
	$(INC)/filehdr.h \
	$(INC)/unistd.h \
	$(INC)/stdio.h \
	$(INC)/stdlib.h \
	$(INC)/string.h \
	$(INC)/sys/dirent.h \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/mkdev.h \
	$(INC)/sys/param.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/types.h \
	$(FRC)
