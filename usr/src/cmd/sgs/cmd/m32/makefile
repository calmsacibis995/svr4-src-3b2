#	Copyright (c) 1988 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)sgs-cmd:m32/makefile	1.24"

# makefile for 3b2 cc command
# we build and install
# two versions, 1 for the source product, that allows both
# -Kmau and -Kfpe, and 1 for the binary, that allows only -Kmau

ROOT=
SGS=m32
OWN=bin
GRP=bin
CC=cc
CFLAGS=-O
LDLIBS =
LINT=lint
LINTFLAGS=
LINK_MODE=

CCSBIN=$(ROOT)/usr/ccs/bin
CCSLIB=$(ROOT)/usr/ccs/lib

INC=$(ROOT)/usr/include
INCSYS=$(ROOT)/usr/include
LIB=$(ROOT)/lib
SGSBASE=../..
MACH=m32
MACHINC=$(SGSBASE)/inc/$(MACH)
COMINC=$(SGSBASE)/inc/common
LIBELF=$(SGSBASE)/libelf/$(MACH)/libelf.a
COMDIR=../common

BIN=$(ROOT)/bin
INS=$(SGSBASE)/sgs.install
INSDIR=$(CCSBIN)

STRIP=strip

HFILES=$(MACHINC)/paths.h $(MACHINC)/sgs.h

SOURCES=$(COMDIR)/cc.c

SRC_OBJ=cc.o
BIN_OBJ=cc_kmau.o
OBJECTS=$(SRC_OBJ) $(BIN_OBJ)

PRODUCTS=cc cc_kmau

ARCH=AR32W
DEFLIST=-D$(ARCH) -DM32

INCLIST=-I$(MACHINC) -I$(INC) -I$(INCSYS) -I$(COMINC)

CC_CMD=$(CC) $(CFLAGS) $(DEFLIST) $(INCLIST)

all:	$(PRODUCTS)
	
$(PRODUCTS): 	$(OBJECTS)
	$(CC) $(BIN_OBJ) $(LINK_MODE) -o cc_kmau
	$(CC) $(SRC_OBJ) $(LINK_MODE) -o cc

cc.o: 	$(HFILES) $(COMDIR)/cc.c
	$(CC_CMD) -c -DSRC_PROD $(COMDIR)/cc.c 

cc_kmau.o: 	$(HFILES) $(COMDIR)/cc.c
	cp $(COMDIR)/cc.c ./cc_kmau.c
	$(CC_CMD) -c ./cc_kmau.c 

install:	$(PRODUCTS)
	cp cc cc.bak
	$(STRIP) cc
	cp cc_kmau cc_kmau.bak
	$(STRIP) cc_kmau
	/bin/sh $(INS) 755 $(OWN) $(GRP) $(INSDIR)/$(SGS)cc cc
	/bin/sh $(INS) 755 $(OWN) $(GRP) $(INSDIR)/$(SGS)cc_kmau cc_kmau
	mv cc.bak cc
	mv cc_kmau.bak cc_kmau

lintit:	$(HFILES) $(SOURCES)
	$(LINT) $(LINTFLAGS) $(DEFLIST) $(INCLIST) $(SOURCES)

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f $(PRODUCTS) cc_kmau.c
