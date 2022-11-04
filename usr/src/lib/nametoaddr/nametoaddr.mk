#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)nametoaddr:nametoaddr.mk	1.2"
#
# makefile for name to address mapping dynamic linking libraries.
#

ROOT =
TESTDIR = .
INC = $(ROOT)/usr/include
INS = install
CFLAGS = -O
COMPONENTS = npack straddr tcpip
FRC =

all:
	@for i in $(COMPONENTS);\
		do cd $$i;\
		make -f $$i.mk "MAKE=$(MAKE)" "AS=$(AS)" "CC=$(CC)" "LD=$(LD)" "FRC=$(FRC)" "INC=$(INC)" "MORECPP=$(MORECPP)" "DASHO=$(DASHO)";\
		cd ..;\
	done;

install: 
	for i in $(COMPONENTS);\
		do cd $$i;\
		echo $$i;\
		make install -f $$i.mk "MAKE=$(MAKE)" "AS=$(AS)" "CC=$(CC)" "LD=$(LD)" "FRC=$(FRC)" "INC=$(INC)" "MORECPP=$(MORECPP)" "DASHO=$(DASHO)";\
		cd ..;\
	done;

clean:
	for i in $(COMPONENTS);\
	do\
		cd $$i;\
		echo $$i;\
		make -f $$i.mk clean;\
		cd .. ;\
	done

clobber:
	for i in $(COMPONENTS);\
		do cd $$i;\
		echo $$i;\
		make -f $$i.mk clobber;\
		cd .. ;\
	done

FRC:
