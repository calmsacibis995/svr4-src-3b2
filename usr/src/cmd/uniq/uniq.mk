#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)uniq:uniq.mk	1.2"
#	Makefile for uniq

ROOT =

DIR = $(ROOT)/usr/bin

INC = $(ROOT)/usr/include

LDFLAGS = $(SHLIBS) 

CFLAGS = -O -I$(INC)

INS = install

MAKEFILE = uniq.mk

MAINS = uniq

STRIP = strip

OBJECTS =  uniq.o

SOURCES =  uniq.c

ALL:		$(MAINS)

uniq:	$(SOURCES)
	$(CC) $(CFLAGS)  -o uniq  uniq.c   $(LDFLAGS)

strip:
	$(STRIP) $(MAINS)
clean:
	rm -f $(OBJECTS)

clobber:
	rm -f $(OBJECTS) $(MAINS)


all : ALL

install: ALL
	$(INS) -f $(DIR)  -m 0555 -u bin -g bin $(MAINS)

