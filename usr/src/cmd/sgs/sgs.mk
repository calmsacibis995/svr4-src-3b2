#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

#ident	"@(#)sgs:sgs.mk.um32	1.12.2.2"
#	3b15/3b5/3b2 Cross-SGS Global Makefile
#	PATHEDIT MUST BE RUN BEFORE THIS MAKEFILE IS USED!
#
#

YACC=yacc
LEX=lex
CC=cc
CCSBIN=
CCSLIB=/usr/ccs/lib
USRLIB=
SGS=
MAC=BMAC

all:	install libs
	echo "Finished building and installing both tools and libraries."

libs: 
	cd ../../lib; make -f .lib.mk install YACC="$(YACC)" LEX="$(LEX)" CC="$(CC)" MAC="$(MAC)" CCSBIN="$(CCSBIN)"
	cd xenv/m32; \
		 make libclobber; \
		 make libs; \
		 make libinstall; \
		 make libclobber;
		echo "Installed default libraries."

install:  sgs	
	cd xenv/m32; make CCSBIN=$(ROOT)/usr/ccs/bin CCSLIB=$(ROOT)/usr/ccs/lib install YACC="$(YACC)" LEX="$(LEX)" CC="$(CC)"
	cd xenv/m32; make libcopy

sgs:	
	cd xenv/m32; $(MAKE) all YACC="$(YACC)" LEX="$(LEX)" CC="$(CC)"

shrink:	clobber
	if [ true ] ; \
	then \
		cd ../../lib; make -f .lib.mk clobber ; \
	fi

lintit:
	cd xenv/m32; make lintit 

libslintit:
	cd ../../lib; make .lib.mk lintit

clean:
	cd xenv/m32; make clean

clobber:
	cd xenv/m32; make clobber
