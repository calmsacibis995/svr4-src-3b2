#	Copyright (c) 1988 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ld:m32/Makefile	1.25"

# makefile for 3B2 ELF version of the link editor

ROOT=
CCSBIN=$(ROOT)/usr/ccs/bin
CCSLIB=$(ROOT)/usr/ccs/lib
SGS=m32
OWN=bin
GRP=bin
CC=cc
CFLAGS= -O
LDLIBS=
LINK_MODE=
LINT=lint
LINTFLAGS=
MOD=755
SGSBASE=../..
INS=$(SGSBASE)/sgs.install
INSDIR=$(CCSBIN)
STRIP=strip

COMINC=$(SGSBASE)/inc/common
COMDIR=../common
INC=$(ROOT)/usr/include
LIBELF=$(SGSBASE)/libelf/$(MACH)/libelf.a
MACH=m32
MACHINC=$(SGSBASE)/inc/$(MACH)

DEFLIST= -DPORTAR
INCLIST=-I$(COMDIR) -I$(COMINC) -I$(MACHDIR) -I$(MACHINC)
LDFLAGS=
MACHDIR=../$(MACH)

LOCALHEADS= $(COMDIR)/globals.h  $(COMDIR)/macros.h $(MACHDIR)/machdep.h

HFILES=$(LOCALHEADS) $(MACHINC)/paths.h $(MACHINC)/sgs.h $(INC)/sys/stat.h

SOURCES= \
	$(COMDIR)/args.c \
	$(COMDIR)/files.c \
	$(COMDIR)/globals.c \
	$(COMDIR)/libs.c \
	$(MACHDIR)/machrel.c \
	$(COMDIR)/main.c \
	$(COMDIR)/map.c \
	$(COMDIR)/outfile.c \
	$(COMDIR)/relocate.c \
	$(COMDIR)/sections.c \
	$(COMDIR)/syms.c \
	$(COMDIR)/update.c \
	$(COMDIR)/util.c


OBJECTS= \
	args.o \
	files.o \
	globals.o \
	libs.o \
	machrel.o \
	main.o \
	map.o \
	outfile.o \
	relocate.o \
	sections.o \
	syms.o \
	util.o \
	update.o

PRODUCTS= ld

all: $(PRODUCTS)

$(PRODUCTS): $(OBJECTS)
	$(CC) -o $(PRODUCTS) $(LDFLAGS) $(CFLAGS) \
		$(OBJECTS) $(LINK_MODE) $(LIBELF)

args.o:		$(COMDIR)/args.c $(COMDIR)/globals.h $(MACHDIR)/machdep.h
		$(CC) -c $(CFLAGS) $(DEFLIST) $(INCLIST) $(COMDIR)/args.c

files.o: 	$(COMDIR)/files.c $(COMDIR)/globals.h $(MACHDIR)/machdep.h
		$(CC) -c $(CFLAGS) $(DEFLIST) $(INCLIST) $(COMDIR)/files.c

globals.o:	$(COMDIR)/globals.c $(COMDIR)/globals.h
		$(CC) -c $(CFLAGS) $(DEFLIST) $(INCLIST) $(COMDIR)/globals.c

libs.o:		$(COMDIR)/libs.c $(COMDIR)/globals.h $(MACHDIR)/machdep.h
		$(CC) -c $(CFLAGS) $(DEFLIST) $(INCLIST) $(COMDIR)/libs.c

main.o:		$(COMDIR)/main.c $(COMDIR)/globals.h \
		$(COMDIR)/macros.h $(MACHDIR)/machdep.h
		$(CC) -c $(CFLAGS) $(DEFLIST) $(INCLIST) $(COMDIR)/main.c

map.o:		$(COMDIR)/map.c $(COMDIR)/globals.h $(MACHDIR)/machdep.h $(INC)/sys/stat.h
		$(CC) -c $(CFLAGS) $(DEFLIST) $(INCLIST) $(COMDIR)/map.c
			
relocate.o:	$(COMDIR)/relocate.c $(COMDIR)/globals.h  \
		$(COMDIR)/macros.h $(MACHDIR)/machdep.h
		$(CC) -c $(CFLAGS) $(DEFLIST) $(INCLIST) $(COMDIR)/relocate.c

outfile.o:	$(COMDIR)/outfile.c $(COMDIR)/globals.h \
		$(COMDIR)/macros.h $(MACHDIR)/machdep.h
		$(CC) -c $(CFLAGS) $(DEFLIST) $(INCLIST) $(COMDIR)/outfile.c

sections.o:	$(COMDIR)/sections.c $(COMDIR)/globals.h $(COMDIR)/macros.h $(MACHDIR)/machdep.h
		$(CC) -c $(CFLAGS) $(DEFLIST) $(INCLIST) $(COMDIR)/sections.c

syms.o:		$(COMDIR)/syms.c $(COMDIR)/globals.h \
		$(COMDIR)/macros.h $(MACHDIR)/machdep.h
		$(CC) -c $(CFLAGS) $(DEFLIST) $(INCLIST) $(COMDIR)/syms.c

update.o:	$(COMDIR)/update.c $(COMDIR)/globals.h $(MACHDIR)/machdep.h
		$(CC) -c $(CFLAGS) $(DEFLIST) $(INCLIST) $(COMDIR)/update.c

util.o:		$(COMDIR)/util.c $(COMDIR)/globals.h $(MACHDIR)/machdep.h
		$(CC) -c $(CFLAGS) $(DEFLIST) $(INCLIST) $(COMDIR)/util.c

machrel.o:	$(MACHDIR)/machrel.c $(COMDIR)/globals.h  \
		$(COMDIR)/macros.h $(MACHDIR)/machdep.h
		$(CC) -c $(CFLAGS) $(DEFLIST) $(INCLIST) $(MACHDIR)/machrel.c

install:all
	cp ld ld.bak
	$(STRIP) ld
	/bin/sh $(INS) $(MOD) $(OWN) $(GRP) $(INSDIR)/$(SGS)ld ld
	mv ld.bak ld

lintit:	$(LOCALHEADS) $(SOURCES)
	$(LINT) $(LINTFLAGS) $(SOURCES)

printit: $(LOCALHEADS) $(SOURCES)
	pr -n -f $(LOCALHEADS) $(SOURCES) > print.out

clean:
	-rm -f $(OBJECTS)

clobber:	clean
	-rm -f $(PRODUCTS)

