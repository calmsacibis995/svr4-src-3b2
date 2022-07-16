#	Copyright (c) 1988 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)lorder:m32/makefile	1.11.1.2"
#	LORDER MAKEFILE
#
#

ROOT=
SGS=m32
OWN=bin
GRP=bin
CC=cc
CFLAGS=-O
LDLIB=
LINT=lint
LINTFLAGS=

SGSBASE=../..
CMDBASE=..
COMDIR=../common
CCSBIN=$(ROOT)/usr/ccs/bin

INS=$(SGSBASE)/sgs.install
INSDIR=$(CCSBIN)

HFILES=

SOURCES=

OBJECTS=

PRODUCTS=lorder

SED=/bin/sed
SEDFLAGS=-e
SED_CMD=$(SED) $(SEDFLAGS)

all:		lorder

lorder:		$(COMDIR)/lorder
		-rm -f lorder
		$(SED_CMD) "s/SGS/$(SGS)/" < $(COMDIR)/lorder > lorder
 
install:	all
		/bin/sh $(INS) 755 $(OWN) $(GRP) $(INSDIR)/$(SGS)lorder lorder

clean:
	rm -f $(PRODUCTS)

clobber: clean

lintit:

