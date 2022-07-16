#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)nametoaddr:resolv/resolv.mk	1.1"

#	Makefile for resolv.so

ROOT=
DIR=		$(ROOT)/usr/lib
INC=		$(ROOT)/usr/include

CFLAGS=		-O -I$(INC) -DSYSV -Kpic
LFLAGS=		-dy -G -ztext
STRIP=		strip
SIZE=		size

MAKEFILE=	resolv.mk
LIBNAME=	resolv.so
OBJ=		resolv.o dns_db.o res.o
SRC=		resolv.c dns_db.c res.c

all:		$(LIBNAME)

resolv.so:	$(OBJ) $(RESOBJ)
		$(CC) -o $(LIBNAME) $(CFLAGS) $(LFLAGS) $(OBJ) $(LDFLAGS)

clean:
		rm -f $(OBJ)

clobber:
		rm -f $(OBJ) $(LIBNAME)

install:	all
		install -f $(DIR) resolv.so

size:		all
		$(SIZE) $(LIBNAME)

strip:		all
		$(STRIP) $(LIBNAME)

#
# header dependencies
#
dns_db.o:	dns_db.c \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/netinet/in.h \
		$(INC)/ctype.h \
		$(INC)/netdb.h \
		$(INC)/stdio.h \
		$(INC)/errno.h \
		$(INC)/arpa/inet.h \
		$(INC)/arpa/nameser.h \
		$(INC)/resolv.h \
		$(INC)/netdir.h \
		$(INC)/string.h \
		$(FRC)


res.o:		res.c \
		$(INC)/sys/types.h \
		$(INC)/stdio.h \
		$(INC)/netinet/in.h \
		$(INC)/sys/select.h \
		$(INC)/arpa/nameser.h \
		$(INC)/resolv.h \
		$(INC)/sys/param.h \
		$(INC)/sys/time.h \
		$(INC)/sys/socket.h \
		$(INC)/sys/uio.h \
		$(INC)/errno.h \
		$(FRC)


resolv.o:	resolv.c \
		$(INC)/stdio.h \
		$(INC)/ctype.h \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/sys/sockio.h \
		$(INC)/netinet/in.h \
		$(INC)/netdb.h \
		$(INC)/tiuser.h \
		$(INC)/netconfig.h \
		$(INC)/netdir.h \
		$(INC)/string.h \
		$(INC)/fcntl.h \
		$(INC)/sys/param.h \
		$(INC)/sys/errno.h \
		$(INC)/sys/utsname.h \
		$(INC)/net/if.h \
		$(INC)/stropts.h \
		$(INC)/sys/ioctl.h \
		$(INC)/sys/syslog.h \
		$(FRC)
