#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)finc:finc.mk	1.3"
DIR = $(ROOT)/usr/sbin
INC = $(ROOT)/usr/include
LDFLAGS = -s
CFLAGS = -O -I$(INC)
INS = install
SYMLINK = :

all: finc

finc: finc.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o finc finc.o $(ROOTLIBS)

install: all
	-rm -rf $(ROOT)/etc/finc
	$(INS) -f $(DIR) -m 0555 -u bin -g bin finc
	-$(SYMLINK) /usr/sbin/finc $(ROOT)/etc/finc
clean:
	rm -f *.o

clobber: clean
	rm -f finc
