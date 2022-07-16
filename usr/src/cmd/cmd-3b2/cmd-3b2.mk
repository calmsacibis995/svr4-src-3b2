#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cmd-3b2:cmd-3b2.mk	1.14.3.1"
#
# cmd-3b2.mk
#
INC = $(ROOT)/usr/include
CFLAGS = -O -I$(INC) -Du3b2
LDFLAGS = -s
INS = install
MAKE = make
LIBELF = -lelf
FRC =

DFILES =\
	absunix.o \
	dlabelit.o \
	dswap.o \
	fsys.o \
	pdinfo.o \
	pnewboot.o \
	scat.o \
	ttyset.o \
	wtvtoc.o

NOSHFILES =\
	absunix |\
	dlabelit |\
	dswap |\
	fsys |\
	pdinfo |\
	pnewboot |\
	scat |\
	ttyset |\
	wtvtoc


FILES = absunix dlabelit dswap fsys pdinfo pnewboot scat ttyset wtvtoc
all:  mkfs $(DFILES) cpio lbin

install:  all 
	cp absunix \
	   dlabelit \
	   dswap \
	   fsys \
	   pdinfo \
	   pnewboot \
	   scat \
	   ttyset \
	   wtvtoc	$(ROOT)/mkfs/inst/bin


.c.o:
	@-case $* in \
		$(NOSHFILES))\
			echo "\t$(CC) $(CFLAGS) $(LDFLAGS) -o $* $< $(LIBELF) $(NOSHLIBS)";\
			$(CC) $(CFLAGS) $(LDFLAGS) -o $* $< $(LIBELF) $(NOSHLIBS);\
			;;\
		*)\
			;;\
		esac


mkfs:
		if [ ! -d $(ROOT)/mkfs ];then mkdir $(ROOT)/mkfs; fi;


cpio:
		find inst -print | cpio -pdum $(ROOT)/mkfs

lbin:
		$(MAKE) -f lbin.mk install

clean:
	-rm -f *.o

clobber:    clean
	-rm -f $(FILES)

FRC:

#
# Header dependencies
#

absunix.o: absunix.c \
	$(INC)/aouthdr.h \
	$(INC)/fcntl.h \
	$(INC)/filehdr.h \
	$(INC)/scnhdr.h \
	$(FRC)

dlabelit.o: dlabelit.c \
	$(INC)/sys/param.h \
        $(INC)/sys/types.h \
	$(INC)/signal.h \
	$(INC)/sys/filsys.h \
	$(FRC)



dswap.o: dswap.c \
	$(INC)/errno.h \
	$(INC)/fcntl.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/sys3b.h \
	$(INC)/sys/types.h \
	$(INC)/sys/vtoc.h \
	$(FRC)



fsys.o: fsys.c \
	$(INC)/fcntl.h \
	$(INC)/sys/filsys.h \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(FRC)



pdinfo.o: pdinfo.c \
	$(INC)/errno.h \
	$(INC)/fcntl.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/id.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/mkdev.h \
	$(INC)/sys/types.h \
	$(INC)/sys/vtoc.h \
	$(FRC)


soat.o: scat.c \
	$(INC)/fcntl.h
	$(FRC)


ttyset.o: ttyset.c \
	$(INC)/sys/termio.h \
	$(INC)/termio.h \
	$(FRC)

pnewboot.o: pnewboot.c \
	$(INC)/a.out.h \
	$(INC)/aouthdr.h \
	$(INC)/fcntl.h \
	$(INC)/filehdr.h \
	$(INC)/linenum.h \
	$(INC)/nlist.h \
	$(INC)/reloc.h \
	$(INC)/scnhdr.h \
	$(INC)/storclass.h \
	$(INC)/syms.h \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/vtoc.h \
	$(FRC)

wtvtoc.o: wtvtoc.c \
	$(INC)/fcntl.h \
	$(INC)/stdio.h \
	$(INC)/errno.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/vtoc.h \
	$(FRC)
