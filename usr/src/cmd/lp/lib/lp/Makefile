#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)nlp:lib/lp/Makefile	1.18"
#
# Makefile for LP support routines
#


TOP	=	../..

include ../../common.mk


DEBUG	=	-g

CFLAGS	=	$(DEBUG) -I$(LPINC) -I$(INC)

SRCS	= \
		Syscalls.c \
		addlist.c \
		addstring.c \
		appendlist.c \
		alerts.c \
		charset.c \
		cs_strcmp.c \
		cs_strncmp.c \
		dellist.c \
		dashos.c \
		dirs.c \
		duplist.c \
		files.c \
		freelist.c \
		getlist.c \
		getname.c \
		getpaths.c \
		getspooldir.c \
		isterminfo.c \
		joinlist.c \
		lenlist.c \
		lp_errno.c\
		makepath.c \
		makestr.c \
		mergelist.c \
		next.c \
		printlist.c \
		sdn.c \
		sprintlist.c \
		searchlist.c \
		set_charset.c \
		set_pitch.c \
		set_size.c \
		sop.c \
		strip.c \
		syntax.c \
		tidbit.c \
		tinames.c \
		wherelist.c \
		which.c

OBJS	=	$(SRCS:.c=.o)

LINTS	=

LIBNAME	=	lp

include ../libcommon.mk
