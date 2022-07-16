#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)netinet:netinet/netinet.mk	1.5"
#
#
#  		PROPRIETARY NOTICE (Combined)
#  
#  This source code is unpublished proprietary information
#  constituting, or derived under license from AT&T's Unix(r) System V.
#  In addition, portions of such source code were derived from Berkeley
#  4.3 BSD under license from the Regents of the University of
#  California.
#  
#  
#  
#  		Copyright Notice 
#  
#  Notice of copyright on this source code product does not indicate 
#  publication.
#  
#  	(c) 1986,1987,1988,1989  Sun Microsystems, Inc.
#  	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
#  	          All rights reserved.
#
INC = /usr/include
INSDIR = /boot
MASTERD = ../master.d
MKBOOT = mkboot
PFLAGS = -DSYSV -Du3b2 -D_KERNEL $(MORECPP) -I$(INC)
DASHO = -O
CFLAGS = $(DASHO) $(PFLAGS)
CC = cc
LD = ld

PRODUCTS = ARP APP ICMP IP LLCLOOP RAWIP TCP UDP

uts: all

all:	$(PRODUCTS)

ARP:	FRC
	$(MAKE) -f arp.mk "MAKE=$(MAKE)" "MASTERD=$(MASTERD)" "AS=$(AS)" "CC=$(CC)" "LD=$(LD)" "FRC=$(FRC)" "INC=$(INC)" "MORECPP=$(MORECPP)" "DASHO=$(DASHO)" ARP

APP:	FRC
	$(MAKE) -f arp.mk "MAKE=$(MAKE)" "MASTERD=$(MASTERD)" "AS=$(AS)" "CC=$(CC)" "LD=$(LD)" "FRC=$(FRC)" "INC=$(INC)" "MORECPP=$(MORECPP)" "DASHO=$(DASHO)" APP

ICMP:	FRC
	$(MAKE) -f ip.mk "MAKE=$(MAKE)" "MASTERD=$(MASTERD)" "AS=$(AS)" "CC=$(CC)" "LD=$(LD)" "FRC=$(FRC)" "INC=$(INC)" "MORECPP=$(MORECPP)" "DASHO=$(DASHO)" ICMP

IP:	FRC
	$(MAKE) -f ip.mk "MAKE=$(MAKE)" "MASTERD=$(MASTERD)" "AS=$(AS)" "CC=$(CC)" "LD=$(LD)" "FRC=$(FRC)" "INC=$(INC)" "MORECPP=$(MORECPP)" "DASHO=$(DASHO)" IP

LLCLOOP:	FRC
	$(MAKE) -f llcloop.mk "MAKE=$(MAKE)" "MASTERD=$(MASTERD)" "AS=$(AS)" "CC=$(CC)" "LD=$(LD)" "FRC=$(FRC)" "INC=$(INC)" "MORECPP=$(MORECPP)" "DASHO=$(DASHO)" LLCLOOP

RAWIP:	FRC
	$(MAKE) -f ip.mk "MAKE=$(MAKE)" "MASTERD=$(MASTERD)" "AS=$(AS)" "CC=$(CC)" "LD=$(LD)" "FRC=$(FRC)" "INC=$(INC)" "MORECPP=$(MORECPP)" "DASHO=$(DASHO)" RAWIP

TCP:	FRC
	$(MAKE) -f tcp.mk "MAKE=$(MAKE)" "MASTERD=$(MASTERD)" "AS=$(AS)" "CC=$(CC)" "LD=$(LD)" "FRC=$(FRC)" "INC=$(INC)" "MORECPP=$(MORECPP)" "DASHO=$(DASHO)" TCP

UDP:	FRC
	$(MAKE) -f udp.mk "MAKE=$(MAKE)" "MASTERD=$(MASTERD)" "AS=$(AS)" "CC=$(CC)" "LD=$(LD)" "FRC=$(FRC)" "INC=$(INC)" "MORECPP=$(MORECPP)" "DASHO=$(DASHO)" UDP

install:	all
	cp $(PRODUCTS) $(INSDIR)

clean:
	rm -f *.o

clobber:	clean
	rm -f $(PRODUCTS)

FRC:
