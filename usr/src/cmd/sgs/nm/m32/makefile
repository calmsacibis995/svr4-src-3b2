#	Copyright (c) 1988 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)nm:m32/makefile	1.6"

#
#
#	NM MAKEFILE - M32 VERSION
#
#

ROOT=
SGS=m32
OWN=bin
GRP=bin
CC=cc
CFLAGS=-O
LINT=lint
LINTFLAGS=
LINK_MODE=

INC=$(ROOT)/usr/include
INCSYS=$(ROOT)/usr/include
LIB=$(ROOT)/lib
SGSBASE=../..
MACH=m32
MACHINC=$(SGSBASE)/inc/$(MACH)
COMINC=$(SGSBASE)/inc/common
LIBELF=$(SGSBASE)/libelf/$(MACH)/libelf.a
COMDIR=../common
LDLIBS=
CCSBIN=$(ROOT)/usr/ccs/bin
CCSLIB=$(ROOT)/usr/ccs/lib

BIN=$(ROOT)/bin
INS=$(SGSBASE)/sgs.install
INSDIR=$(CCSBIN)

STRIP=strip

HFILES=$(MACHINC)/sgs.h $(MACHINC)/paths.h \
       $(COMINC)/libelf.h

SOURCES=$(COMDIR)/nm.c
OBJECTS=nm.o
PRODUCTS=nm
ARFORMAT=PORTAR

ARCH=
DEFLIST=-D$(ARFORMAT)
INCLIST=-I$(MACHINC) -I$(COMINC) 
CC_CMD=$(CC) $(CFLAGS) $(DEFLIST) $(INCLIST)



all:	nm

nm: 	$(OBJECTS) $(LIBELF) $(LDLIBS)
	$(CC) $(OBJECTS) $(LIBELF) $(LDLIBS) $(LINK_MODE) -o nm

nm.o: 	$(HFILES) $(COMDIR)/nm.c
	$(CC_CMD) -c $(COMDIR)/nm.c 


install:	$(INSDIR)/$(SGS)nm 

$(INSDIR)/$(SGS)nm:	nm
	cp nm nm.bak
	$(STRIP) nm
	/bin/sh $(INS) 755 $(OWN) $(GRP) $(INSDIR)/$(SGS)nm nm
	mv nm.bak nm

lintit:	$(HFILES) $(SOURCES)
	$(LINT) $(LINTFLAGS) $(DEFLIST) $(INCLIST) $(SOURCES)

clean:
	-rm -f $(OBJECTS)

clobber: clean
	-rm -f $(PRODUCTS)

