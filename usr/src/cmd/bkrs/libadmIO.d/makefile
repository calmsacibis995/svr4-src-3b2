#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)bkrs:libadmIO.d/makefile	1.3"

SRC=libadmIO.c ctc_io.c
OBJ=$(SRC:.c=.o)
INS=install
INC=$(ROOT)/usr/include
LOCAL=hdrs
GENERAL=../hdrs
BKBIN=$(ROOT)/usr/sadm/bkup/bin
OAMBASE=$(ROOT)/usr/sadm/sysadm
SBIN=$(ROOT)/sbin
USBIN=$(ROOT)/usr/sbin
ETCBKUP=$(ROOT)/etc/bkup
LIBDIR=$(ROOT)/usr/lib/libadmIO
USRLIB=$(ROOT)/usr/lib
CC=$(PFX)cc
FLAGS=
CFLAGS=-I $(LOCAL) -I $(GENERAL) -I $(INC) $(FLAGS)
LIBRARY = libadmIO.a
LDFLAGS=$(LDLIBPATH)
MAKE=make
STRIP=$(PFX)strip
LORDER=$(PFX)lorder
LINT=$(PFX)lint

PRODUCTS=libadmIO.a CTC

all: $(PRODUCTS)

libadmIO.a: libadmIO.o
	$(AR) cr $(@) libadmIO.o

CTC: ctc_io.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(@) ctc_io.o $(LDLIBS)

touch: 
	touch $(SRC)

clean:
	rm -f $(OBJ)

clobber: clean
	rm -f $(PRODUCTS)

lintit:
	$(LINT) $(CFLAGS) $(LFLAGS) $(SRC)

strip:	$(PRODUCTS)
	$(STRIP) $(PRODUCTS)

install: $(PRODUCTS)
	$(INS) -f $(USRLIB) -m 644 $(LIBRARY)
	$(INS) -f $(LIBDIR) -u root -g sys -m 0700 CTC

$(LIBRARY)(libadmIO.o):	$(INC)/errno.h \
		 	$(INC)/sys/errno.h \
			$(INC)/sys/types.h \
			$(INC)/sys/stat.h \
			$(INC)/fcntl.h \
			$(INC)/sys/fcntl.h \
			$(INC)/sys/sys3b.h \
			$(INC)/stdio.h \
			$(INC)/string.h \
			$(INC)/ftw.h \
			$(GENERAL)/libadmIO.h 

ctc_io.o:	$(INC)/stdio.h \
		$(INC)/sys/stat.h \
		$(INC)/sys/vtoc.h \
		$(INC)/sys/ct.h \
		$(INC)/fcntl.h \
		$(INC)/errno.h \
		$(INC)/sys/stropts.h \
		$(GENERAL)/libadmIO.h
