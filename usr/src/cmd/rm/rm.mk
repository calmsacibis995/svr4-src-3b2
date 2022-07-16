#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)rm:rm.mk	1.5.2.1"

#	Makefile for rm

ROOT =

DIR = $(ROOT)/usr/bin

INC = $(ROOT)/usr/include

LDFLAGS = -s $(PERFLIBS)

INS = install

CFLAGS = -O -I$(INC)

STRIP = strip

SIZE = size

MAKEFILE = rm.mk

MAINS = rm

OBJECTS =  rm.o

SOURCES =  rm.c

ALL:		$(MAINS)

rm:	rm.o
	$(CC) $(CFLAGS) -o rm rm.o $(LDFLAGS)

rm.o:		 $(INC)/stdio.h \
		 $(INC)/fcntl.h \
		 $(INC)/sys/types.h \
		 $(INC)/sys/stat.h \
		 $(INC)/sys/dirent.h 

GLOBALINCS = $(INC)/fcntl.h \
	$(INC)/stdio.h \
	$(INC)/sys/dirent.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/types.h 


clean:
	rm -f $(OBJECTS)

clobber:
	rm -f $(OBJECTS) $(MAINS)

newmakefile:
	makefile -m -f $(MAKEFILE)  -s INC $(INC)
#bottom#

all : ALL

install: ALL
	$(INS) -f $(DIR) -m 00555 -u bin -g bin rm

size: ALL
	$(SIZE) $(MAINS)

strip: ALL
	$(STRIP) $(MAINS)

#	These targets are useful but optional

partslist:
	@echo $(MAKEFILE) $(SOURCES) $(LOCALINCS)  |  tr ' ' '\012'  |  sort

productdir:
	@echo $(DIR) | tr ' ' '\012' | sort

product:
	@echo $(MAINS)  |  tr ' ' '\012'  | \
	sed 's;^;$(DIR)/;'

srcaudit:
	@fileaudit $(MAKEFILE) $(LOCALINCS) $(SOURCES) -o $(OBJECTS) $(MAINS)