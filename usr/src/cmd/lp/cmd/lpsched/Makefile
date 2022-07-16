#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)nlp:cmd/lpsched/Makefile	1.10.1.2"
#
# Makefile for LP scheduler trio
#


TOP	=	../..

include ../../common.mk


DEBUG	=	-O

##########
#
# If you add directories other than one level deep
# you'll have to change the code below.
##########
CMDDIRS= \
		./lpNet \
		./lpsched


all:		cmds

install clean clobber:
	for dir in $(CMDDIRS); \
	do \
		cd $$dir; \
		$(MAKE) $@; \
		cd ..; \
	done

strip:
	for dir in $(CMDDIRS); \
	do \
		cd $$dir; \
		$(MAKE) STRIP=$(STRIP) strip; \
		cd ..; \
	done

cmds:
	for dir in $(CMDDIRS); \
	do \
		cd $$dir; \
		$(MAKE) DEBUG="$(DEBUG)"; \
		cd ..; \
	done

lint:
	for dir in $(CMDDIRS); \
	do \
		cd $$dir; \
		$(MAKE) DEBUG="$(DEBUG)" LINT="$(LINT)" lint; \
		cd ..; \
	done
