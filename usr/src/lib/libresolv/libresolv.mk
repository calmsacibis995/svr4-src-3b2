#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libresolv:libresolv.mk	1.1"

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
UCBINC=		$(ROOT)/usr/ucbinclude
USRLIB=		$(ROOT)/usr/lib
INSTALL=	install

CFLAGS=		$(DASHO) $(MORECPP) -I$(UCBINC) -I$(INC)

LIBNAME=	libresolv.a

LIBOBJS= \
		$(LIBNAME)(gthostnamadr.o) \
		$(LIBNAME)(res_comp.o) \
		$(LIBNAME)(res_debug.o) \
		$(LIBNAME)(res_init.o) \
		$(LIBNAME)(res_mkquery.o) \
		$(LIBNAME)(res_query.o) \
		$(LIBNAME)(res_send.o) \
		$(LIBNAME)(sethostent.o) \
		$(LIBNAME)(strcasecmp.o)

all:		$(LIBNAME)

install:	all
		$(INSTALL) -f $(USRLIB) -m 0444 -u bin -g bin $(LIBNAME)

$(LIBNAME):	$(LIBOBJS)


clean:
		rm -f *.o

clobber:	clean
		rm -f $(LIBNAME)

#
# to do -- Header dependencies
#
