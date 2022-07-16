#	Copyright (c) 1988 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cscope:m32/makefile	1.8"
PROGRAM = $(SGS)cscope
SGS=m32
ROOT=
CCSBIN=$(ROOT)/usr/ccs/bin
CCSLIB=$(ROOT)/usr/ccs/lib
INSDIR=$(CCSBIN)
BASE= ../..
#
BIN = $(ROOT)/usr/bin
M32INC=$(BASE)/sgs/inc/m32
COMINC=$(BASE)/sgs/inc/common
INC=$(ROOT)/usr/include
INCSYS=$(ROOT)/usr/include
COMCS= ../common
#
CC=cc
STRIP = strip
ARCH = AR32WR
DEFLIST= -DCCS -D$(ARCH) -DM32
INCLIST= -I$(COMCS) -I$(COMINC) -I$(INC)
LIBELF=
LINK_MODE=
#
LINT= lint
LFLAGS=
OWN= bin
GRP= bin
#
LIBS= -lcurses -ll -lgen
MORECFLAGS=
CFLAGS= -O -c $(MORECFLAGS) 

OBJ = main.o dir.o crossref.o scanner.o lookup.o command.o display.o find.o \
	edit.o exec.o help.o history.o input.o mouse.o
LIBOBJ = alloc.o basename.o compath.o egrep.o getwd.o logdir.o mygetenv.o \
	  mypopen.o vpaccess.o vpfopen.o vpinit.o
OBJECTS = $(OBJ) $(LIBOBJ)

CFILES =  $(COMCS)/main.c $(COMCS)/dir.c $(COMCS)/crossref.c \
	scanner.c $(COMCS)/lookup.c $(COMCS)/command.c \
	$(COMCS)/display.c $(COMCS)/find.c $(COMCS)/edit.c \
	$(COMCS)/exec.c $(COMCS)/help.c $(COMCS)/history.c $(COMCS)/input.c \
	$(COMCS)/mouse.c $(COMCS)/alloc.c $(COMCS)/basename.c \
	$(COMCS)/compath.c egrep.c $(COMCS)/getwd.c $(COMCS)/logdir.c \
	$(COMCS)/mygetenv.c $(COMCS)/mypopen.c $(COMCS)/vpaccess.c \
	$(COMCS)/vpfopen.c $(COMCS)/vpinit.c

all:	$(PROGRAM)

$(PROGRAM): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS) $(LINK_MODE) $(LIBS)

main.o:	$(COMCS)/constants.h $(COMCS)/version.h \
	$(COMCS)/main.c
	$(CC) $(CFLAGS) $(DEFLIST) $(INCLIST) $(COMCS)/main.c

dir.o:	$(COMCS)/constants.h $(COMCS)/ndir.h $(COMCS)/vp.h \
	$(COMCS)/dir.c
	$(CC) $(CFLAGS) $(DEFLIST) $(INCLIST) $(COMCS)/dir.c
	
crossref.o:	$(COMCS)/constants.h \
	$(COMCS)/crossref.c
	$(CC) $(CFLAGS) $(DEFLIST) $(INCLIST) $(COMCS)/crossref.c

scanner.c:	$(COMCS)/constants.h $(COMCS)/scanner.l
	$(LEX) -n -t $(COMCS)/scanner.l >scanner.c
scanner.o:	scanner.c
	$(CC) $(CFLAGS) $(DEFLIST) $(INCLIST) scanner.c

lookup.o:	$(COMCS)/constants.h \
	$(COMCS)/lookup.c
	$(CC) $(CFLAGS) $(DEFLIST) $(INCLIST) $(COMCS)/lookup.c

command.o:	$(COMCS)/constants.h \
	$(COMCS)/command.c
	$(CC) $(CFLAGS) $(DEFLIST) $(INCLIST) $(COMCS)/command.c

display.o:	$(M32INC)/sgs.h $(COMCS)/version.h $(COMCS)/constants.h \
	$(COMCS)/display.c
	$(CC) $(CFLAGS) $(DEFLIST) -I$(M32INC) $(INCLIST) $(COMCS)/display.c

find.o:	$(COMCS)/constants.h \
	$(COMCS)/find.c
	$(CC) $(CFLAGS) $(DEFLIST) $(INCLIST) $(COMCS)/find.c

edit.o: $(COMCS)/constants.h \
	$(COMCS)/edit.c
	$(CC) $(CFLAGS) $(DEFLIST) $(INCLIST) $(COMCS)/edit.c

exec.o:	$(COMCS)/constants.h \
	$(COMCS)/exec.c
	$(CC) $(CFLAGS) $(DEFLIST) $(INCLIST) $(COMCS)/exec.c

help.o:	$(COMCS)/constants.h \
	$(COMCS)/help.c
	$(CC) $(CFLAGS) $(DEFLIST) $(INCLIST) $(COMCS)/help.c

history.o:	$(COMCS)/constants.h \
	$(COMCS)/history.c
	$(CC) $(CFLAGS) $(DEFLIST) $(INCLIST) $(COMCS)/history.c

input.o: $(COMCS)/constants.h \
	$(COMCS)/input.c
	$(CC) $(CFLAGS) $(DEFLIST) $(INCLIST) $(COMCS)/input.c

mouse.o: $(COMCS)/constants.h \
	$(COMCS)/mouse.c
	$(CC) $(CFLAGS) $(DEFLIST) $(INCLIST) $(COMCS)/mouse.c

alloc.o:	$(COMCS)/alloc.c
	$(CC) $(CFLAGS) $(DEFLIST) $(INCLIST) $(COMCS)/alloc.c

basename.o:	$(COMCS)/basename.c
	$(CC) $(CFLAGS) $(DEFLIST) $(INCLIST) $(COMCS)/basename.c

compath.o:	$(COMCS)/compath.c
	$(CC) $(CFLAGS) $(DEFLIST) $(INCLIST) $(COMCS)/compath.c

egrep.c:	$(COMCS)/egrep.y
	$(YACC) $(COMCS)/egrep.y
	mv y.tab.c egrep.c
egrep.o:	egrep.c
	$(CC) $(CFLAGS) $(DEFLIST) $(INCLIST) egrep.c

getwd.o:	$(COMCS)/ndir.h $(COMCS)/getwd.c
	$(CC) $(CFLAGS) $(DEFLIST) $(INCLIST) $(COMCS)/getwd.c

logdir.o:	$(COMCS)/logdir.c
	$(CC) $(CFLAGS) $(DEFLIST) $(INCLIST) $(COMCS)/logdir.c

mygetenv.o:	$(COMCS)/mygetenv.c
	$(CC) $(CFLAGS) $(DEFLIST) $(INCLIST) $(COMCS)/mygetenv.c

mypopen.o:	$(COMCS)/mypopen.c
	$(CC) $(CFLAGS) $(DEFLIST) $(INCLIST) $(COMCS)/mypopen.c

vpaccess.o:	$(COMCS)/vp.h $(COMCS)/vpaccess.c
	$(CC) $(CFLAGS) $(DEFLIST) $(INCLIST) $(COMCS)/vpaccess.c

vpfopen.o:	$(COMCS)/vp.h $(COMCS)/vpfopen.c
	$(CC) $(CFLAGS) $(DEFLIST) $(INCLIST) $(COMCS)/vpfopen.c

vpinit.o:	$(COMCS)/vp.h $(COMCS)/vpinit.c
	$(CC) $(CFLAGS) $(DEFLIST) $(INCLIST) $(COMCS)/vpinit.c

install: all
	$(STRIP) $(PROGRAM)
	cp $(PROGRAM) $(INSDIR)/$(PROGRAM)
	chmod +x $(INSDIR)/$(PROGRAM)
	chgrp $(GRP) $(INSDIR)/$(PROGRAM)
	chown $(OWN) $(INSDIR)/$(PROGRAM)
clean:
	rm -f *.o *.out lex.yy.c y.tab.? scanner.c egrep.c

clobber: clean
	rm -f $(PROGRAM)

strip:	$(PROGRAM)
	-$(STRIP) $(PROGRAM)

lintit:	$(M32INC)/sgs.h $(CFILES)
	$(LINT) $(LFLAGS) $(INCLIST) $(DEFLIST) $(ODEFLIST) $(LIBS) $(CFILES)
