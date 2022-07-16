#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)csplit:csplit.mk	1.4"

#	Makefile for csplit

ROOT =

DIR = $(ROOT)/usr/bin

INC = $(ROOT)/usr/include

LDFLAGS = -s -lgen $(SHLIBS)

CFLAGS = -O -I$(INC)

INS = install

MAKEFILE = csplit.mk

MAINS = csplit

OBJECTS =  csplit.o

SOURCES =  csplit.c

ALL:		$(MAINS)

csplit:	$(SOURCES)
	$(CC) $(CFLAGS)  -o csplit  csplit.c   $(LDFLAGS)

clean:
	rm -f $(OBJECTS)

clobber:
	rm -f $(OBJECTS) $(MAINS)


all : ALL

install: ALL
	$(INS) -f $(DIR)  -m 0555 -u bin -g bin $(MAINS)

