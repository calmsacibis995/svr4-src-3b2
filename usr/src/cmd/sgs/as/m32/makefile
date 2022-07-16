#	Copyright (c) 1988 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)as:m32/makefile	1.10"
#
#	WE32XXX Assembler Makefile
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
CCSBIN=$(ROOT)/usr/ccs/bin
CCSLIB=$(ROOT)/usr/ccs/lib


INC=$(ROOT)/usr/include
INCSYS=$(ROOT)/usr/include
SGSBASE=../..
MACH=m32
MACHINC=$(SGSBASE)/inc/$(MACH)
COMINC=$(SGSBASE)/inc/common
LIBELF=$(SGSBASE)/libelf/$(MACH)/libelf.a
COMDIR=../common
3BAS=../3b
LDLIBS=

INS=$(SGSBASE)/sgs.install
INSDIR=$(ROOT)/lib

STRIP=strip

ARCH=AR32W
DBO=FBO
MAC=ABWRMAC
PCCALL=CALLPCREL
FLEX= -DFLEXNAMES

DEFLIST=-D$(ARCH) -D$(DBO) -D$(PCCALL) $(FLEX) -D$(MAC) -DM32 -DELF_TARGET_M32
INCLIST=-I. -I$(COMDIR) -I$(3BAS) -I$(COMINC) -I$(MACHINC) -I$(INC)
ENV=


CC_CMD=$(CC) -c $(CFLAGS) $(DEFLIST) $(INCLIST) $(ENV)
LD_CMD=$(CC) $(LDFLAGS) 
STRIP_CMD=$(STRIP)
LINT_CMD=$(LINT) -c $(CFLAGS) $(DEFLIST) $(INCLIST) $(ENV)

LDFLAGS=$(FFLAG)

LIBM=-lm

B30=



o=o

HFILES = expand2.h instab.h  program.h systems.h gendefs.h maugen.h symbols2.h \
	$(COMDIR)/codeout.h $(COMDIR)/expr.h    $(COMDIR)/symbols.h \
	$(COMDIR)/expand.h  $(COMDIR)/section.h \
	$(MACHINC)/paths.h $(MACHINC)/sgs.h  $(MACHINC)/tv.h \
	$(COMINC)/libelf.h
SOURCES = addr2.c  expand2.c    gencode.c    operand.c    swagen.c  \
	  $(COMDIR)/expand1.c float.c maugen.c program.c     $(3BAS)/as_ctype.c \
	  $(COMDIR)/addr1.c   $(COMDIR)/pass2.c \
	  $(COMDIR)/code.c  $(COMDIR)/symbols.c \
	  $(COMDIR)/codeout.c   $(COMDIR)/obj.c   $(COMDIR)/symlist.c \
	  $(COMDIR)/errors.c    $(COMDIR)/pass0.c  \
	  $(COMDIR)/expr.c  $(COMDIR)/pass1.c 

OBJS =	as_ctype.$o pass0.$o program.$o operand.$o gencode.$o pass1.$o \
	symbols.$o code.$o expand1.$o expand2.$o errors.$o addr2.$o float.$o \
	pass2.$o addr1.$o codeout.$o  obj.$o symlist.$o swagen.$o  \
	maugen.$o  expr.$o  

PRODUCTS = as

.SUFFIXES: .ln

.c.ln:
	$(LINT_CMD) $<

.c.o:
	$(CC_CMD) $<

#################################################################
#
# EXTERNAL TARGETS:
#
#	all:		build the assembler in this directory
#	as:		build the assembler in this directory
#	lintit:		lint the assembler
#	install:	install the assembler and m4 files in
#			CCSBIN and CCSLIB owned by OWN/GRP
#	clean:		remove all intermediate files
#	shrink:		remove all intermediate files and "as"
#	clobber:	remove all intermediate files and "as"
#
#################################################################

all: $(PRODUCTS)

lintit: ops.in $(SOURCES) $(HFILES) 
	$(LINT) $(LINTFLAGS) $(DEFLIST) $(INCLIST) $(SOURCES) > lint.out 

install:	$(CCSBIN)/$(SGS)as $(CCSLIB)/cm4defs 

clean:
	-rm -f $(OBJS:$o=o) $(OBJS:$o=ln)
	-rm -f lint.out ind.out typ.out mnm.out sem.out make_tables ops.out parse.h

shrink clobber:	clean
	-rm -f as

#################################################################
#	INTERNAL TARGETS
#################################################################

$(PRODUCTS): $(OBJS)
	$(LD_CMD) -o as $(OBJS) $(LINK_MODE) $(LIBM) $(LIBELF) $(LDLIBS) 


$(CCSBIN)/$(SGS)as:	as
	cp as as.bak
	-$(STRIP_CMD) as
	sh $(SGSBASE)/sgs.install 755 $(OWN) $(GRP) $(CCSBIN)/$(SGS)as as
	mv as.bak as

$(CCSLIB)/cm4defs:	cm4defs
	sh $(SGSBASE)/sgs.install 664 $(OWN) $(GRP) $(CCSLIB)/cm4defs cm4defs


#########################################################################

as_ctype.$o:	$(3BAS)/as_ctype.c
		$(CC_CMD) $(3BAS)/as_ctype.c

addr1.$o:	$(COMDIR)/addr1.c
		$(CC_CMD) $(COMDIR)/addr1.c

code.$o:	$(COMDIR)/code.c
		$(CC_CMD) $(COMDIR)/code.c

codeout.$o:	$(COMDIR)/codeout.c
		$(CC_CMD) $(COMDIR)/codeout.c

errors.$o:	$(COMDIR)/errors.c
		$(CC_CMD) $(COMDIR)/errors.c

expr.$o:	$(COMDIR)/expr.c
		$(CC_CMD) $(COMDIR)/expr.c


obj.$o:		$(COMDIR)/obj.c
		$(CC_CMD) $(COMDIR)/obj.c

pass0.$o:	$(COMDIR)/pass0.c
		$(CC_CMD) $(COMDIR)/pass0.c

pass1.$o:	$(COMDIR)/pass1.c
		$(CC_CMD) $(COMDIR)/pass1.c

pass2.$o:	$(COMDIR)/pass2.c
		$(CC_CMD) $(COMDIR)/pass2.c 

symbols.$o:	$(COMDIR)/symbols.c
		$(CC_CMD) $(COMDIR)/symbols.c

symlist.$o:	$(COMDIR)/symlist.c
		$(CC_CMD) $(COMDIR)/symlist.c

expand1.$o:	$(COMDIR)/expand1.c
		$(CC_CMD) $(COMDIR)/expand1.c


addr2.$o:	addr2.c
expand2.$o:	expand2.c
float.$o:	float.c
gencode.$o:	gencode.c
maugen.$o:	maugen.c

operand.$o:	operand.c

program.$o:	program.c
		$(CC_CMD) $(B30) program.c

swagen.$o:	swagen.c

make_tables:	$(3BAS)/make_tables.c
		cc -o make_tables $(3BAS)/make_tables.c	# MUST USE resident cc

ind.out typ.out mnm.out sem.out:	ops.in make_tables
		./make_tables <ops.in

ops.out:
	  echo >ops.out

parse.h:
	  echo >parse.h


#########################################
#	HEADER FILE DEPENDENCIES	#
#########################################

#	DIRECTORY 3b/

as_ctype.$o:	$(3BAS)/as_ctype.h

#	DIRECTORY common/

addr1.$o:	gendefs.h
addr1.$o:	systems.h
addr1.$o:	$(COMDIR)/symbols.h
addr1.$o:	$(COMDIR)/codeout.h
addr1.$o:	$(COMINC)/libelf.h

code.$o:	gendefs.h
code.$o:	systems.h
code.$o:	$(COMDIR)/symbols.h
code.$o:	$(COMDIR)/codeout.h
code.$o:	$(COMDIR)/expr.h
code.$o:	$(COMDIR)/section.h

codeout.$o:	gendefs.h
codeout.$o:	systems.h
codeout.$o:	$(COMDIR)/symbols.h
codeout.$o:	$(COMDIR)/section.h
codeout.$o:	$(COMDIR)/expr.h
codeout.$o:	$(COMINC)/libelf.h

errors.$o:	gendefs.h
errors.$o:	systems.h

expr.$o:	gendefs.h
expr.$o:	systems.h
expr.$o:	$(COMDIR)/symbols.h


obj.$o:		gendefs.h
obj.$o:		systems.h
obj.$o:		$(COMDIR)/section.h
obj.$o:		$(COMDIR)/symbols.h
obj.$o:		$(COMINC)/libelf.h
obj.$o:		$(MACHINC)/sgs.h

pass0.$o:	$(MACHINC)/paths.h
pass0.$o:	$(MACHINC)/sgs.h
pass0.$o:	gendefs.h
pass0.$o:	systems.h

pass1.$o:	$(COMDIR)/codeout.h
pass1.$o:	$(COMDIR)/section.h
pass1.$o:	$(COMDIR)/symbols.h
pass1.$o:	$(MACHINC)/paths.h
pass1.$o:	gendefs.h
pass1.$o:	systems.h

pass2.$o:	systems.h
pass2.$o:	gendefs.h
pass2.$o:	$(COMDIR)/codeout.h
pass2.$o:	$(COMDIR)/section.h
pass2.$o:	$(COMDIR)/symbols.h
pass2.$o:	$(COMINC)/libelf.h

symbols.$o:	gendefs.h
symbols.$o:	symbols2.h
symbols.$o:	systems.h
symbols.$o:	$(COMDIR)/symbols.h

symlist.$o:	gendefs.h
symlist.$o:	systems.h
symlist.$o:	$(COMDIR)/symbols.h


addr2.$o:	instab.h
addr2.$o:	program.h
addr2.$o:	gendefs.h
addr2.$o:	systems.h
addr2.$o:	expand2.h
addr2.$o:	$(COMDIR)/expr.h
addr2.$o:	$(COMDIR)/symbols.h
addr2.$o:	$(COMDIR)/section.h
addr2.$o:	$(COMDIR)/codeout.h
addr2.$o:	$(COMDIR)/expand.h
addr2.$o:	$(COMINC)/libelf.h

expand1.$o:	systems.h
expand1.$o:	expand2.h
expand1.$o:	$(COMDIR)/expand.h
expand1.$o:	$(COMDIR)/symbols.h
expand1.$o:	$(COMDIR)/codeout.h

expand2.$o:	expand2.h

float.$o:	instab.h
float.$o:	systems.h
float.$o:	program.h
float.$o:	$(COMDIR)/expr.h
float.$o:	$(COMDIR)/symbols.h

gencode.$o:	instab.h
gencode.$o:	systems.h
gencode.$o:	program.h
gencode.$o:	gendefs.h
gencode.$o:	expand2.h
gencode.$o:	$(COMDIR)/expr.h
gencode.$o:	$(COMDIR)/symbols.h
gencode.$o:	$(COMDIR)/expand.h
gencode.$o:	$(COMDIR)/expr.h

maugen.$o:	maugen.h
maugen.$o:	expand2.h
maugen.$o:	instab.h
maugen.$o:	systems.h
maugen.$o:	program.h
maugen.$o:	gendefs.h
maugen.$o:	$(COMDIR)/expr.h
maugen.$o:	$(COMDIR)/symbols.h
maugen.$o:	$(COMDIR)/expand.h

operand.$o:	program.h
operand.$o:	instab.h
operand.$o:	systems.h
operand.$o:	$(COMDIR)/symbols.h
operand.$o:	$(COMDIR)/expr.h
operand.$o:	$(3BAS)/as_ctype.h

program.$o:	ind.out typ.out mnm.out sem.out
program.$o:	program.h
program.$o:	instab.h
program.$o:	systems.h
program.$o:	gendefs.h
program.$o:	$(3BAS)/as_ctype.h
program.$o:	$(COMDIR)/expr.h
program.$o:	$(COMDIR)/symbols.h
program.$o:	$(COMDIR)/section.h
program.$o:	$(COMDIR)/codeout.h
program.$o:	$(COMINC)/libelf.h
program.$o:	$(MACHINC)/sgs.h

swagen.$o:	instab.h
swagen.$o:	systems.h
swagen.$o:	gendefs.h
swagen.$o:	program.h
swagen.$o:	$(COMDIR)/symbols.h
swagen.$o:	$(COMDIR)/expr.h

