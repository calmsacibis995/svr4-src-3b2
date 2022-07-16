#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)frec:frec.mk	1.3"
DIR = $(ROOT)/usr/sbin
INC = $(ROOT)/usr/include
LDFLAGS = -s
CFLAGS = -O -I$(INC)
INS = install
SYMLINK = :

all: frec

frec: frec.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o frec frec.o $(ROOTLIBS)

install: all
	-rm -rf $(ROOT)/etc/frec
	$(INS) -f $(DIR) -m 0555 -u bin -g bin frec
	-$(SYMLINK) /usr/sbin/frec $(ROOT)/etc/frec

clean:
	rm -f *.o

clobber: clean
	rm -f frec
