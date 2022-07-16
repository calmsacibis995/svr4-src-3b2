#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)newboot:newboot.mk	1.13"
#
#		Copyright 1984 AT&T
#

ROOT =
INC = $(ROOT)/usr/include
INCSYS = $(ROOT)/usr/include
CFLAGS = -O -I$(INC) -I$(INCSYS) -Uu3b -Uvax -Updp11 -Uu3b5 -Du3b2
LDFLAGS = -s
SYMLINK = :
MAKE = make "AS=$(AS)" "CC=$(CC)" "LD=$(LD)"
INS = install
FRC =
ELFLIB = -lelf

all:	newboot

newboot:
	$(CC) $(CFLAGS) $(LDFLAGS) -o newboot newboot.c $(ELFLIB) $(NOSHLIBS)

install: all
	-rm -f $(ROOT)/etc/newboot
	$(INS) -f $(ROOT)/usr/sbin -m 0555 -u bin -g bin newboot
	-$(SYMLINK) /usr/sbin/newboot $(ROOT)/etc/newboot

clean:
	-rm -f newboot.o

clobber: clean
	-rm -f newboot

FRC:

#
# Header dependencies
#

newboot: newboot.c \
	$(INC)/a.out.h \
	$(INC)/aouthdr.h \
	$(INC)/fcntl.h \
	$(INC)/filehdr.h \
	$(INC)/libelf.h \
	$(INC)/linenum.h \
	$(INC)/nlist.h \
	$(INC)/unistd.h \
	$(INC)/reloc.h \
	$(INC)/scnhdr.h \
	$(INC)/stdio.h \
	$(INC)/stdlib.h \
	$(INC)/storclass.h \
	$(INC)/syms.h \
	$(INCSYS)/sys/param.h \
	$(INCSYS)/sys/types.h \
	$(INCSYS)/sys/stat.h \
	$(INCSYS)/sys/vtoc.h \
	$(FRC)
