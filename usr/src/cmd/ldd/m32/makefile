#	Copyright (c) 1988 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ldd:m32/makefile	1.3"

# makefile for m32 version of ldd (List Dynamic Dependencies)

SGSBASE=/ci5/b2/usr/src/cmd/sgs
#SGSBASE=../../sgs
CMDBASE=../..
INS=$(CMDBASE)/install/install.sh
COMDIR=..
MACHDIR=../m32
SOURCES= $(COMDIR)/ldd.c
HFILES= $(MACHDIR)/machdep.h
OBJECTS= ldd.o
INCLIST=-I. -I$(COMINC)
COMINC=$(SGSBASE)/inc/common
DEFLIST=
PRODUCTS=$(SGS)ldd
LIBELF= -lelf
LINK_MODE=

OBJECTS= ldd.o

all:	$(OBJECTS)
	$(CC) -o $(PRODUCTS) $(CFLAGS) $(LINK_MODE) $(OBJECTS) $(LIBELF)

ldd.o:	$(COMDIR)/ldd.c $(HFILES)
	$(CC) -c $(CFLAGS) $(DEFLIST) $(INCLIST) $(COMDIR)/ldd.c

install:	all
	cp $(PRODUCTS) $(PRODUCTS).bak
	$(STRIP) $(PRODUCTS)
	/bin/sh $(INS) -f $(INSDIR) $(PRODUCTS)
	mv $(PRODUCTS).bak $(PRODUCTS)

lintit:	$(SOURCES)
	$(LINT) $(LINTFLAGS) $(INCLIST) $(DEFLIST) $(SOURCES)

clean:
	-rm -f $(OBJECTS)

clobber:	clean
	-rm -f $(PRODUCTS)
