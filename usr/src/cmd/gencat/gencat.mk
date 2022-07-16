#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)gencat:gencat.mk	1.3"

INC=$(ROOT)/usr/include
CFLAGS=-O -I$(INC)

BIN=$(ROOT)/usr/bin

OBJS=	gencat.o msg_conv.o cat_misc.o cat_build.o cat_mmp_dump.o


gencat:	$(OBJS)
	$(CC) -o gencat $(CFLAGS) $(OBJS) $(SHLIBS)

install : gencat
	install -f $(BIN) -m 00555 -u bin -g bin gencat

clean:
	rm -f *.o gencat
	
clobber:
	rm -f *.o gencat gencat
	
.c.o :
	$(CC) -c $(CFLAGS) $<
