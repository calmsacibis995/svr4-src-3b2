#	Copyright (c) 1988 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libld:m32/makefile	1.9.2.1"
#	WE32000 LIBLD MAKEFILE
#
#
ROOT=
SGS=m32
OWN= bin
GRP= bin
CC=cc
CFLAGS=-O
LDLIBS=
LINT=lint
LINTFLAGS=

SGSBASE	=  ../..
CMDBASE = ..
COMINC= $(SGSBASE)/inc/common
COMDIR= ../common
CCSLIB=$(ROOT)/usr/ccs/lib

INS=$(SGSBASE)/sgs.install
INSDIR=$(CCSLIB)

STRIP=strip


HFILES= $(COMINC)/filehdr.h $(COMINC)/ldfcn.h $(COMINC)/scnhdr.h \
	$(COMINC)/syms.h $(COMINC)/storclass.h $(COMINC)/linenum.h \
	$(COMDIR)/lddef.h \
	$(COMDIR)/synsyms.h

SOURCES= $(COMDIR)/ldaclose.c $(COMDIR)/ldaopen.c $(COMDIR)/ldclose.c \
	$(COMDIR)/ldahread.c $(COMDIR)/ldlseek.c $(COMDIR)/ldnlseek.c \
	$(COMDIR)/ldnrseek.c $(COMDIR)/ldnsseek.c $(COMDIR)/ldnshread.c \
	$(COMDIR)/ldopen.c $(COMDIR)/ldrseek.c $(COMDIR)/ldlread.c \
	$(COMDIR)/ldsseek.c $(COMDIR)/ldshread.c $(COMDIR)/ldfhread.c \
	$(COMDIR)/ldtbseek.c $(COMDIR)/ldohseek.c $(COMDIR)/ldtbread.c \
	$(COMDIR)/ldtbindex.c $(COMDIR)/allocldptr.c \
	$(COMDIR)/freeldptr.c $(COMDIR)/vldldptr.c $(COMDIR)/sgetl.c \
	$(COMDIR)/sputl.c $(COMDIR)/ldgetname.c

OBJECTS = ldaclose.o ldaopen.o ldclose.o ldahread.o ldlseek.o ldnlseek.o \
	ldnrseek.o ldnsseek.o ldnshread.o ldopen.o ldrseek.o ldlread.o \
	ldsseek.o ldshread.o ldfhread.o ldtbseek.o ldohseek.o ldtbread.o \
	ldtbindex.o ldgetname.o allocldptr.o freeldptr.o vldldptr.o \
	sgetl.o sputl.o

PRODUCTS=libld.a

ARCH    = AR32W
FLEX    = -DFLEXNAMES
ARFORMAT = PORTAR
#	   PORTAR  = 6.0 portable ascii format


DEFLIST = -D$(ARFORMAT) -D$(ARCH) $(FLEX)
INCLIST = -I$(COMDIR) -I$(COMINC)
CC_CMD  = $(CC) -c $(CFLAGS) $(DEFLIST) $(INCLIST)

AR      = ar
ARFLAGS = r
AR_CMD  = $(AR) $(ARFLAGS)

all:	libld.a
 
libld.a: $(OBJECTS)
	rm -f libld.a
	$(AR_CMD) libld.a $(OBJECTS)

 
allocldptr.o:	$(COMINC)/filehdr.h $(COMINC)/ldfcn.h \
		$(COMDIR)/lddef.h $(COMDIR)/synsyms.h \
		$(COMDIR)/allocldptr.c
	$(CC_CMD) $(COMDIR)/allocldptr.c
 
freeldptr.o:	$(COMINC)/filehdr.h $(COMINC)/ldfcn.h \
		$(COMDIR)/synsyms.h $(COMDIR)/lddef.h \
		$(COMDIR)/freeldptr.c
	$(CC_CMD) $(COMDIR)/freeldptr.c

vldldptr.o:	$(COMINC)/filehdr.h $(COMINC)/ldfcn.h \
		$(COMDIR)/synsyms.h $(COMDIR)/lddef.h \
		$(COMDIR)/vldldptr.c
	$(CC_CMD) $(COMDIR)/vldldptr.c
 
ldaclose.o:	$(COMINC)/filehdr.h $(COMINC)/ldfcn.h \
		$(COMDIR)/synsyms.h $(COMDIR)/ldaclose.c
	$(CC_CMD) $(COMDIR)/ldaclose.c
 
ldaopen.o:	$(COMINC)/filehdr.h $(COMINC)/ldfcn.h \
		$(COMDIR)/synsyms.h $(COMDIR)/ldaopen.c
	$(CC_CMD) $(COMDIR)/ldaopen.c
 
ldclose.o:	$(COMINC)/filehdr.h $(COMINC)/ldfcn.h \
		$(COMDIR)/synsyms.h $(COMDIR)/ldclose.c
	$(CC_CMD) $(COMDIR)/ldclose.c
 
ldahread.o:	$(COMINC)/filehdr.h $(COMINC)/ldfcn.h \
		$(COMDIR)/synsyms.h $(COMDIR)/ldahread.c
	$(CC_CMD) $(COMDIR)/ldahread.c
 
ldlseek.o:	$(COMINC)/filehdr.h $(COMINC)/scnhdr.h \
		$(COMDIR)/synsyms.h $(COMINC)/ldfcn.h \
		$(COMDIR)/ldlseek.c
	$(CC_CMD) $(COMDIR)/ldlseek.c
 
ldnlseek.o:	$(COMINC)/filehdr.h $(COMINC)/scnhdr.h \
		$(COMDIR)/synsyms.h $(COMINC)/ldfcn.h \
		$(COMDIR)/ldnlseek.c
	$(CC_CMD) $(COMDIR)/ldnlseek.c
 
ldnrseek.o:	$(COMINC)/filehdr.h $(COMINC)/scnhdr.h \
		$(COMDIR)/synsyms.h $(COMINC)/ldfcn.h \
		$(COMDIR)/ldnrseek.c
	$(CC_CMD) $(COMDIR)/ldnrseek.c
 
ldnsseek.o:	$(COMINC)/filehdr.h $(COMINC)/scnhdr.h \
		$(COMDIR)/synsyms.h $(COMINC)/ldfcn.h \
		$(COMDIR)/ldnsseek.c
	$(CC_CMD) $(COMDIR)/ldnsseek.c
 
ldnshread.o:	$(COMINC)/filehdr.h $(COMINC)/scnhdr.h \
		$(COMDIR)/synsyms.h $(COMINC)/ldfcn.h \
		$(COMDIR)/ldnshread.c
	$(CC_CMD) $(COMDIR)/ldnshread.c
 
ldopen.o:	$(COMINC)/filehdr.h $(COMINC)/ldfcn.h \
		$(COMDIR)/synsyms.h $(COMDIR)/ldopen.c 
	$(CC_CMD) $(COMDIR)/ldopen.c
 
ldrseek.o:	$(COMINC)/filehdr.h $(COMINC)/scnhdr.h \
		$(COMDIR)/synsyms.h $(COMINC)/ldfcn.h $(COMDIR)/ldrseek.c
	$(CC_CMD) $(COMDIR)/ldrseek.c
 
ldtbread.o:	$(COMINC)/filehdr.h $(COMINC)/syms.h \
		$(COMDIR)/synsyms.h $(COMINC)/storclass.h \
		$(COMINC)/ldfcn.h $(COMDIR)/ldtbread.c
	$(CC_CMD) $(COMDIR)/ldtbread.c
 
ldsseek.o:	$(COMINC)/filehdr.h $(COMINC)/scnhdr.h \
		$(COMDIR)/synsyms.h $(COMINC)/ldfcn.h $(COMDIR)/ldsseek.c
	$(CC_CMD) $(COMDIR)/ldsseek.c
 
ldshread.o:	$(COMINC)/filehdr.h $(COMINC)/scnhdr.h \
		$(COMDIR)/synsyms.h $(COMINC)/ldfcn.h \
		$(COMDIR)/ldshread.c
	$(CC_CMD) $(COMDIR)/ldshread.c
 
ldfhread.o:	$(COMINC)/filehdr.h $(COMINC)/ldfcn.h \
		$(COMDIR)/synsyms.h $(COMDIR)/ldfhread.c
	$(CC_CMD) $(COMDIR)/ldfhread.c
 
ldtbseek.o:	$(COMINC)/ldfcn.h $(COMINC)/filehdr.h \
		$(COMDIR)/synsyms.h $(COMDIR)/ldtbseek.c
	$(CC_CMD) $(COMDIR)/ldtbseek.c
 
ldohseek.o:	$(COMINC)/ldfcn.h $(COMINC)/filehdr.h \
		$(COMDIR)/synsyms.h $(COMDIR)/ldohseek.c
	$(CC_CMD) $(COMDIR)/ldohseek.c
 
ldtbindex.o:	$(COMINC)/ldfcn.h $(COMINC)/filehdr.h \
		$(COMDIR)/synsyms.h $(COMINC)/syms.h \
		$(COMINC)/storclass.h $(COMDIR)/ldtbindex.c
	$(CC_CMD) $(COMDIR)/ldtbindex.c
 
ldlread.o:	$(COMINC)/ldfcn.h $(COMINC)/filehdr.h \
		$(COMDIR)/synsyms.h $(COMINC)/scnhdr.h $(COMINC)/syms.h \
		$(COMINC)/linenum.h $(COMINC)/storclass.h \
		$(COMDIR)/ldlread.c
	$(CC_CMD) $(COMDIR)/ldlread.c
 
sgetl.o:	$(COMDIR)/synsyms.h $(COMDIR)/sgetl.c
	$(CC_CMD) $(COMDIR)/sgetl.c
 
sputl.o:	$(COMDIR)/synsyms.h $(COMDIR)/sputl.c
	$(CC_CMD) $(COMDIR)/sputl.c
 
ldgetname.o:	$(COMINC)/ldfcn.h $(COMINC)/filehdr.h $(COMINC)/syms.h \
		$(COMINC)/storclass.h $(COMDIR)/synsyms.h \
		$(COMDIR)/ldgetname.c
	$(CC_CMD) $(COMDIR)/ldgetname.c
 
install: 	all
	/bin/sh $(INS) 644 $(OWN) $(GRP) $(INSDIR)/libld.a libld.a

lintit: $(HFILES) $(SOURCES)
	$(LINT) $(LINTFLAGS) $(DEFLIST) $(INCLIST) $(SOURCES)

clean:
	rm -f $(OBJECTS)

clobber:	clean
	rm -f $(PRODUCTS)

llib-lld.ln:	$(COMDIR)/llib-lld.c
	$(LINT) $(COMDIR)/llib-lld.c -c
