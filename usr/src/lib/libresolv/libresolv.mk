#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libresolv:libresolv.mk	1.2"

#
# +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
# 		PROPRIETARY NOTICE (Combined)
# 
# This source code is unpublished proprietary information
# constituting, or derived under license from AT&T's UNIX(r) System V.
# In addition, portions of such source code were derived from Berkeley
# 4.3 BSD under license from the Regents of the University of
# California.
# 
# 
# 
# 		Copyright Notice 
# 
# Notice of copyright on this source code product does not indicate 
# publication.
# 
# 	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
# 	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
# 	          All rights reserved.
#  

DASHO=		-O
MORECPP=	-DDEBUG -DSYSV
INC=		$(ROOT)/usr/include
INCSYS=		$(ROOT)/usr/include/sys
USRLIB=		$(ROOT)/usr/lib
INSTALL=	install
LORDER=		lorder
TSORT=		tsort

CFLAGS=		$(DASHO) $(MORECPP) -I$(INC)

LIBNAME=	libresolv.a

OBJS=		gthostnamadr.o res_comp.o res_debug.o res_init.o \
		res_mkquery.o res_query.o res_send.o sethostent.o \
		strcasecmp.o

all:		$(LIBNAME)

install:	all
		$(INSTALL) -f $(USRLIB) -m 0444 -u bin -g bin $(LIBNAME)

$(LIBNAME):	$(OBJS)
		$(AR) crv $(LIBNAME) `$(LORDER) $(OBJS) | $(TSORT)`

clean:
		rm -f *.o

clobber:	clean
		rm -f $(LIBNAME)

#
# Header dependencies
#
gthostnamadr.o:	gthostnamadr.c \
		$(INC)/sys/param.h \
		$(INC)/sys/socket.h \
		$(INC)/netinet/in.h \
		$(INC)/ctype.h \
		$(INC)/netdb.h \
		$(INC)/stdio.h \
		$(INC)/errno.h \
		$(INC)/arpa/inet.h \
		$(INC)/arpa/nameser.h \
		$(INC)/resolv.h \
		res.h \
		$(FRC)

res_comp.o:	res_comp.c \
		$(INC)/sys/types.h \
		$(INC)/stdio.h \
		$(INC)/arpa/nameser.h \
		res.h \
		$(FRC)

res_debug.o:	res_debug.c \
		$(INC)/sys/types.h \
		$(INC)/netinet/in.h \
		$(INC)/stdio.h \
		$(INC)/arpa/nameser.h \
		res.h \
		$(FRC)

res_init.o:	res_init.c \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/netinet/in.h \
		$(INC)/stdio.h \
		$(INC)/arpa/nameser.h \
		$(INC)/resolv.h \
		res.h \
		$(FRC)

res_mkquery.o:	res_mkquery.c \
		$(INC)/stdio.h \
		$(INC)/sys/types.h \
		$(INC)/netinet/in.h \
		$(INC)/arpa/nameser.h \
		$(INC)/resolv.h \
		res.h \
		$(FRC)

res_query.o:	res_query.c \
		$(INC)/sys/param.h \
		$(INC)/sys/socket.h \
		$(INC)/netinet/in.h \
		$(INC)/ctype.h \
		$(INC)/netdb.h \
		$(INC)/stdio.h \
		$(INC)/errno.h \
		$(INC)/string.h \
		$(INC)/arpa/inet.h \
		$(INC)/arpa/nameser.h \
		$(INC)/resolv.h \
		res.h \
		$(FRC)

res_send.o:	res_send.c \
		$(INC)/sys/param.h \
		$(INC)/sys/time.h \
		$(INC)/sys/socket.h \
		$(INC)/sys/uio.h \
		$(INC)/netinet/in.h \
		$(INC)/stdio.h \
		$(INC)/errno.h \
		$(INC)/arpa/nameser.h \
		$(INC)/resolv.h \
		res.h \
		$(FRC)

sethostent.o:	sethostent.c \
		$(INC)/sys/types.h \
		$(INC)/arpa/nameser.h \
		$(INC)/netinet/in.h \
		$(INC)/resolv.h \
		res.h \
		$(FRC)

strcasecmp.o:	strcasecmp.c \
		res.h \
		$(FRC)
