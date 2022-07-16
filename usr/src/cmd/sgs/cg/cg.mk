#	Copyright (c) 1988 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)nifg:cg/cg.mk	1.5"
#	Copyright (c) 1984 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

# Master makefile for cg.o; main function is to validate the target,
# then cd to the target directory and execute cg.mk there.
# TARGET has no default
TARGET=
# NAILROOT is the root of the nail source tree
NAILROOT=..
# LIB: where cg.o is moved to on make install
LIB=$(NAILROOT)/lib/$(TARGET)
# INCLUDE: where the headers go on make install
INCLUDE=$(NAILROOT)/include/$(TARGET)
MAKE=make

all:	cg.o 

cg.o:	valid_target
	cd $(TARGET); $(MAKE) -f cg.mk cg.o
	cp $(TARGET)/cg.o cg.o

#clean: remove all output files except cg.o
clean:	valid_target
	cd $(TARGET); $(MAKE) -f cg.mk clean

#clobber: remove all output files including cg.o
clobber:	valid_target
	cd $(TARGET); $(MAKE) -f cg.mk clobber
	rm -f cg.o

#install: put cg.o and the headers in appropriate places
install:	cg.o
	mv cg.o $(LIB)/cg.o
	cp common/*.h $(TARGET)/*.h $(INCLUDE)

# valid_target: make sure that TARGET is set to something useful
# kludge to put $(MAKE) in the command string: use the pound sign
# to comment it out.  This forces a valid arg even on make -n.
valid_target:	
	@if test ! "$(TARGET)" ; then \
		echo "cg.mk: No target specified"; \
		exit 1; \
	fi; # $(MAKE)
	@if test ! -r "$(TARGET)/cg.mk"; then \
		echo "cg.mk: Invalid target: $(TARGET)" ; \
		exit 1; \
	fi; # $(MAKE)

#product: echo the names of things that would be built by 'make all'
product:	
	@echo cg.o
