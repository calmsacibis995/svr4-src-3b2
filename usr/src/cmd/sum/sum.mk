#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)sum:sum.mk	1.4"
#	Makefile for sum

ROOT =

DIR = $(ROOT)/usr/bin

SYMLINK = :

INC = $(ROOT)/usr/include

LDFLAGS = $(SHLIBS) 

CFLAGS = -O -I$(INC)

INS = install

MAKEFILE = sum.mk

MAINS = sum

STRIP = strip

OBJECTS =  sum.o

SOURCES =  sum.c

ALL:		$(MAINS)

sum:	$(SOURCES)
	$(CC) $(CFLAGS)  -o sum  sum.c   $(LDFLAGS)

strip:
	$(STRIP) $(MAINS)
clean:
	rm -f $(OBJECTS)

clobber:
	rm -f $(OBJECTS) $(MAINS)


all : ALL

install: ALL
	$(INS) -f $(DIR)  -m 0555 -u bin -g bin $(MAINS)

