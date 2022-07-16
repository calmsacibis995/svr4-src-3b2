#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)comm:comm.mk	1.3"
#	Makefile for comm

ROOT =

DIR = $(ROOT)/usr/bin

INC = $(ROOT)/usr/include

LDFLAGS =

CFLAGS = -O -I$(INC)

INS = install

MAKEFILE = comm.mk

MAINS = comm

STRIP = strip

OBJECTS =  comm.o

SOURCES =  comm.c

ALL:		$(MAINS)

comm:	$(SOURCES)
	$(CC) $(CFLAGS)  -o comm  comm.c   $(LDFLAGS) $(PERFLIBS)

strip:
	$(STRIP) $(MAINS)
clean:
	rm -f $(OBJECTS)

clobber:
	rm -f $(OBJECTS) $(MAINS)


all : ALL

install: ALL
	$(INS) -f $(DIR)  -m 0555 -u bin -g bin $(MAINS)

