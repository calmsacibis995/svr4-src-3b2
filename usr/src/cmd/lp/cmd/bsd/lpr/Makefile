#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)lp:cmd/bsd/lpr/Makefile	1.9"

ROOT	=
RM	=	/bin/rm -f
INS	=	install
STRIP	=	strip
LINT	=	lint
TAGS	=	ctags

OWNER	=	lp
GROUP	=	lp

EMODES	=	0555

LPROOT	=	../../..
LPLIB	=	$(LPROOT)/lib
LPINC	=	$(LPROOT)/include
USRUCB	=	$(ROOT)/usr/ucb
USRLIB	=	$(ROOT)/usr/lib
INC	=	$(ROOT)/usr/include
INCSYS	=	$(INC)/sys

PERFLIBS=	-dn
DEBUG	=	-DDEBUG
XCFLAGS	=	$(DEBUG) -O
CFLAGS	=	-I$(LPINC) -I$(INC) $(XCFLAGS)
LDFLAGS	=	$(PERFLIBS)

LIBACC	=	$(LPLIB)/access/liblpacc.a
LIBBSD	=	$(LPLIB)/bsd/liblpbsd.a
LIBCLS	=	$(LPLIB)/class/liblpcls.a
LIBFLT	=	$(LPLIB)/filters/liblpflt.a
LIBFRM	=	$(LPLIB)/forms/liblpfrm.a
LIBFS	=	$(LPLIB)/fs/liblpfs.a
LIBLP	=	$(LPLIB)/lp/liblp.a
LIBMSG	=	$(LPLIB)/msgs/liblpmsg.a
LIBOAM	=	$(LPLIB)/oam/liblpoam.a
LIBPRT	=	$(LPLIB)/printers/liblpprt.a
LIBREQ	=	$(LPLIB)/requests/liblpreq.a
LIBUSR	=	$(LPLIB)/users/liblpusr.a

LINTACC	=	$(LPLIB)/access/llib-llpacc.ln
LINTACC	=	$(LPLIB)/bsd/llib-llpbsd.ln
LINTCLS	=	$(LPLIB)/class/llib-llpcls.ln
LINTFLT	=	$(LPLIB)/filters/llib-llpflt.ln
LINTFRM	=	$(LPLIB)/forms/llib-llpfrm.ln
LINTFS	=	$(LPLIB)/fs/llib-llpfs.ln
LINTLP	=	$(LPLIB)/lp/llib-llp.ln
LINTMSG	=	$(LPLIB)/msgs/llib-llpmsg.ln
LINTOAM	=	$(LPLIB)/oam/llib-llpoam.ln
LINTPRT	=	$(LPLIB)/printers/llib-llpprt.ln
LINTREQ	=	$(LPLIB)/requests/llib-llpreq.ln
LINTUSR	=	$(LPLIB)/users/llib-llpusr.ln

SRCS	=	lpr.c

OBJS	=	$(SRCS:.c=.o)

CMD	=	lpr

all:	$(CMD)

$(CMD):		$(OBJS)		\
		$(LIBBSD)	\
		$(LIBREQ)	 \
		$(LIBMSG)	\
		$(LIBOAM)	\
		$(LIBLP)
	$(CC) -o $(CMD) $(OBJS) $(LDFLAGS)	\
			$(LIBBSD)	\
			$(LIBREQ)	\
			$(LIBMSG)	\
			$(LIBOAM)	\
			$(LIBLP)	\
			-lgen

install:	all strip
	$(INS) -m $(EMODES) -u $(OWNER) -g $(GROUP) -f $(USRUCB) $(CMD)

clean:
	$(RM) $(OBJS) a.out core

clobber:	clean
	$(RM) $(CMD)

stamp:

strip:
	$(STRIP) $(CMD)


lint:
	$(LINT) $(CFLAGS) $(SRCS)	\
			  $(LINTREQ)	\
			  $(LINTMSG)	\
			  $(LINTOAM)	\
			  $(LINTLP)

ctags:
	$(TAGS) $(SRCS)
