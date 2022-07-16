#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)edittbl:edittbl.mk	1.6.3.2"
TITLE = edittbl

MACHINE = m32
PRODUCTS = edt_data
CFLAGS = -O -I$(INC)
LDFLAGS = -s
ROOT =
INC = $(ROOT)/usr/include
SYMLINK = :
INS = install
DEFS = -Dm32 

all: edittbl

edittbl: edittbl.c
	cc -o edittbl edittbl.c
	./edittbl -t -g -s -d
	> .edt_swapp
	rm -f edittbl
	$(CC) -o edittbl $(DEFS) $(CFLAGS) $(LDFLAGS) edittbl.c $(NOSHLIBS)

install: all
	-rm -f $(ROOT)/etc/edittbl
	-rm -f $(ROOT)/etc/editsa
	$(INS) -f $(ROOT)/dgn -m 0400 -u root -g root edt_data
	$(INS) -f $(ROOT)/dgn -m 0400 -u root -g root .edt_swapp
	$(INS) -f $(ROOT)/usr/sbin -m 0550 -u root -g root editsa
	$(INS) -f $(ROOT)/usr/sbin -m 0755 -u bin -g bin edittbl
	-$(SYMLINK) /usr/sbin/edittbl $(ROOT)/etc/edittbl
	-$(SYMLINK) /usr/sbin/editsa $(ROOT)/etc/editsa

clobber:
	rm -f edittbl edt_data .edt_swapp

clean:

.PRECIOUS: edt_data
