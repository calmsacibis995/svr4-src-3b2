#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)fltboot:fltboot.mk	1.5.3.1"
ROOT =
INC = $(ROOT)/usr/include
SYMLINK = :
CFLAGS = -O -I$(INC)
INS = install
INSDIR = $(ROOT)/usr/sbin

LDFLAGS = -s
FRC =

all: fltboot

install: fltboot
	-rm -f $(ROOT)/etc/fltboot
	$(INS) -f $(INSDIR) -m 0555 -u bin -g bin fltboot 
	-$(SYMLINK) /usr/sbin/fltboot $(ROOT)/etc/fltboot

fltboot:	fltboot.c
		$(CC) $(CFLAGS) -o fltboot fltboot.c $(LDFLAGS) $(SHLIBS)
clean:

clobber: clean
	rm -f fltboot

