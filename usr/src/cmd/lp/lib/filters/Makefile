#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)nlp:lib/filters/Makefile	1.11"
#
# Makefile for filtermgmt(lp) routines
#


TOP	=	../..

include ../../common.mk


DEBUG	=	-g

CFLAGS	=	$(DEBUG) -I$(LPINC) -I$(INC)


SRCS	= \
		conv.c \
		delfilter.c \
		dumpfilters.c \
		freefilter.c \
		getfilter.c \
		filtertable.c \
		insfilter.c \
		loadfilters.c \
		putfilter.c \
		regex.c \
		search.c \
		trash.c

OBJS	=	$(SRCS:.c=.o)

LINTS	= \
		$(LINTLP)

LIBNAME	=	lpflt

include ../libcommon.mk
