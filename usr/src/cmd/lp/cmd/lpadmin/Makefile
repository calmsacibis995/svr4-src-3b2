#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)nlp:cmd/lpadmin/Makefile	1.18"
#
# Makefile for the lpadmin command
#


TOP	=	../..

include ../../common.mk


DEBUG	=	-g -DDEBUG

CFLAGS	=	$(DEBUG) -I. -I$(LPINC) -I$(INC)

LDFLAGS	=	$(SHLIBS)


HDRS	= \
		lpadmin.h

SRCS	= \
		chkopts.c \
		default.c \
		do_align.c \
		do_fault.c \
		do_mount.c \
		do_printer.c \
		do_pwheel.c \
		done.c \
		fromclass.c \
		ismodel.c \
		lpadmin.c \
		options.c \
		output.c \
		rmdest.c \
		send_message.c \
		signals.c \
		startup.c \
		usage.c

OBJS	=	$(SRCS:.c=.o)

LIBS	= \
		$(LIBACC) \
		$(LIBCLS) \
		$(LIBMSG) \
		$(LIBPRT) \
		$(LIBFRM) \
		$(LIBREQ) \
		$(LIBOAM) \
		$(LIBSYS) \
		$(LIBLP)

SYSLIBS	=	-lcurses

LINTS	= \
		$(LINTACC) \
		$(LINTCLS) \
		$(LINTMSG) \
		$(LINTPRT) \
		$(LINTFRM) \
		$(LINTREQ) \
		$(LINTOAM) \
		$(LINTSYS) \
		$(LINTLP)


CMD	=	lpadmin


all:		$(CMD)

install:
	$(INS) -m $(EMODES) -u $(OWNER) -g $(GROUP) -f $(USRSBIN) $(CMD)
	$(SYMLINK) $(USRSBIN)/$(CMD) $(USRLIB)/$(CMD)

clean:
	$(RM) $(OBJS)

clobber:	clean
	$(RM) $(CMD)

strip:
	$(STRIP) $(CMD)

$(CMD):		$(OBJS) $(LIBS)
	$(CC) -o $(CMD) $(OBJS) $(LDFLAGS) $(LIBS) $(SYSLIBS)

$(OBJS):	$(HDRS)

lint:
	$(LINT) -x $(CFLAGS) $(SRCS) $(LINTS)
