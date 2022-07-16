#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)users:users.mk	1.12"

INC=$(ROOT)/usr/include
INCSYS=$(ROOT)/usr/include/sys
LIB=$(ROOT)/lib
USRLIB=$(ROOT)/usr/lib
LIBS=
BIN=$(ROOT)/usr/bin
INSTALL=install
HDRS=$(INC)/fmtmsg.h $(INC)/stdio.h $(INC)/string.h $(INC)/grp.h $(INC)/pwd.h $(INC)/varargs.h
FILE=listusers
INSTALLS=listusers
SRC=users.c
OBJ=$(SRC:.c=.o)
CCLIBLIST=
CFLAGS=-I . -I $(INC) -I $(INCSYS) $(CCLIBLIST) $(CCFLAGS)
LDFLAGS=$(LDFLAGS)

all		: $(FILE) 

install		: all 
		$(INSTALL) -f $(BIN) $(INSTALLS)

clobber		: clean
		rm -f $(FILE)

clean		:
		rm -f $(OBJ)

strip		:
		$(CC) -O -s $(FILE).o -o $(FILE) $(LDLIBPATH) $(CFLAGS)

lintit		:
		for i in $(SRC); \
		do \
		    lint $(CFLAGS) $$i; \
		done

$(FILE)		: $(OBJ) $(LIBS)
		$(CC) -O $(OBJ) -o $(FILE) $(LDLIBPATH) $(CFLAGS)

$(OBJ)		: $(HDRS)
