#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)des:des/des.mk	1.7"

#
#	@(#)des.mk 1.1 88/12/14 SMI
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
#
#	Kernel DES
#
STRIP = strip
MKBOOT = mkboot
MASTERD = ../master.d
BOOTD = .
INC = ..
PFLAGS = -I$(INC) -D_KERNEL $(MORECPP)
CFLAGS = $(DASHO) $(PFLAGS) -DSYSV
DEFLIST =
FRC =

DESOBJ =  des_crypt.o des_soft.o
IDESOBJ =  intldescrypt.o intldes_soft.o


ALL:		
		if [ -s des_crypt.c -a  -s des_soft.c ] ;\
		then \
			make -f des.mk domestic ;\
		fi
		make -f des.mk intl 

domestic:DES

intl:	IDES

DES:	des.o
	$(MKBOOT) -m $(MASTERD) -d $(BOOTD) des.o

IDES:	ides.o
	$(MKBOOT) -m $(MASTERD) -d $(BOOTD) ides.o
lint:
	lint $(CFLAGS) -Dlint *.c

des.o:	$(DESOBJ)
	$(LD) -r -o des.o $(DESOBJ)

ides.o:	$(IDESOBJ)
	$(LD) -r -o ides.o $(IDESOBJ)

clean:
	rm -f *.o

clobber:	clean
	rm -f DES
	rm -f IDES


#
# Header dependencies -- make depend should build these!
#

des_crypt.o: des_crypt.c \
	$(INC)/sys/types.h \
	$(INC)/rpc/des_crypt.h \
	$(INC)/des/des.h

des_soft.o: des_soft.c \
	$(INC)/sys/types.h \
	$(INC)/des/des.h \
	$(INC)/des/softdes.h \
	$(INC)/des/desdata.h \
	$(INC)/sys/debug.h

intldescrypt.o: intldescrypt.c \
	$(INC)/sys/types.h \
	$(INC)/rpc/des_crypt.h \
	$(INC)/des/des.h

intldes_soft.o: intldes_soft.c \
	$(INC)/sys/types.h \
	$(INC)/des/des.h \
	$(INC)/des/softdes.h \
	$(INC)/des/desdata.h \
	$(INC)/sys/debug.h
