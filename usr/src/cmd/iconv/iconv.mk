#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)iconv:iconv.mk	1.2"

INC=$(ROOT)/usr/include
CFLAGS=-O -I$(INC) -I.

BIN=$(ROOT)/usr/bin

OBJS=	iconv.o gettab.o process.o


iconv:	$(OBJS)
	$(CC) -o iconv $(CFLAGS) $(OBJS) $(SHLIBS)

install : iconv
	install -f $(BIN) -m 00555 -u bin -g bin iconv

clean:
	rm -f *.o iconv
	
clobber:
	rm -f *.o iconv iconv
	
$(OBJS): ./kbd.h

.c.o :
	$(CC) -c $(CFLAGS) $<
