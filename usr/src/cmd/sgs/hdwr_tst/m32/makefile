#	Copyright (c) 1988 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)hdwr_tst:m32/makefile	1.2.1.1"

# has_32b  determines whether the 3b2 has B or A chip
# has_mau  determines if the 3b2 has a mau chip

ROOT=
SGS=m32
OWN=bin
GRP=bin
CC=cc
CFLAGS=-O
LDLIBS=
LINT=lint
LINTFLAGS=
LINK_MODE=

CCSBIN=$(ROOT)/usr/ccs/bin
SGSBASE= ../..
M32INC= $(SGSBASE)/inc/m32

INS=$(SGSBASE)/sgs.install
INSDIR=$(CCSBIN)

SOURCES=has_32b.c has_mau.c
OBJECTS=has_32b.o has_mau.o
PRODUCTS=has_32b has_mau

all:		has_32b has_mau

has_32b:
	$(CC) $(CFLAGS) -K fpe has_32b.c $(LDLIBS) $(LINK_MODE) -o has_32b

has_mau:
	$(CC) $(CFLAGS) -K mau has_mau.c $(LDLIBS) $(LINK_MODE) -o has_mau

install:	all
	/bin/sh $(INS) 755 $(OWN) $(GRP) $(CCSBIN)/has_32b has_32b
	/bin/sh $(INS) 755 $(OWN) $(GRP) $(CCSBIN)/has_mau has_mau

lintit:	$(SOURCES)
	$(LINT) $(LINTFLAGS) $(SOURCES)

clean:
	-rm -f $(OBJECTS)

clobber:	clean
	-rm -f $(PRODUCTS)
