#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)boot:boot/olboot/olboot.mk	11.13"

ROOT =
LIB = $(ROOT)/lib
INCSYS = $(ROOT)/usr/include
UINC = $(ROOT)/usr/include
INCLOC = ..
DASHO = -O
CFLAGS = $(DASHO) -I$(INCLOC) -I$(INCSYS) -I$(UINC) $(DBO)
STRIP = strip
LIBNAME = libfm.a
INS = install
SYMLINK = :
FRC =

.c.a:
	$(CC) $(CFLAGS) -c $<
	$(AR) rv $@ $*.o
	-rm -f $*.o

LFILES = \
	$(LIBNAME)(basicio.o)\
	$(LIBNAME)(findfile.o)\
	$(LIBNAME)(findfs.o)\
	$(LIBNAME)(loadfile.o)\
	$(LIBNAME)(data.o)\
	$(LIBNAME)(oloadp.o)

all: flboot olboot

install: all
	$(STRIP) olboot
	-rm -f $(ROOT)/lib/olboot
	-rm -f $(ROOT)/lib/$(LIBNAME)
	$(INS) -f $(ROOT)/usr/lib olboot
	$(INS) -f $(ROOT)/usr/lib $(LIBNAME)
	-$(SYMLINK) $(ROOT)/usr/lib/olboot $(ROOT)/lib/olboot
	-$(SYMLINK) $(ROOT)/usr/lib/$(LIBNAME) $(ROOT)/lib/$(LIBNAME)
	$(STRIP) flboot
	-rm -f $(ROOT)/lib/flboot
	$(INS) -f $(ROOT)/usr/lib flboot
	-$(SYMLINK) $(ROOT)/usr/lib/flboot $(ROOT)/lib/flboot

olboot: lbld\
	olboot.map\
	misc.o\
	lboot.o\
	loadprog.o\
	$(LIBNAME)\
	$(FRC)
	if [ x$(CCSTYPE) = xCOFF ] ; \
	then \
		$(LD) $(LDFLAG) lbld misc.o lboot.o loadprog.o \
			-o olboot $(LIBNAME) -L$(LIB) -lc ; \
	else \
		$(LD) $(LDFLAG) -M olboot.map misc.o lboot.o loadprog.o \
			-o olboot $(LIBNAME) -L$(CCSLIB) -dn -lc ; \
	fi

flboot: lbld\
	olboot.map\
	misc.o\
	flboot.o\
	loadprog.o\
	$(LIBNAME)\
	$(FRC)
	if [ x$(CCSTYPE) = xCOFF ] ; \
	then \
		$(LD) $(LDFLAG) lbld misc.o flboot.o loadprog.o \
			-o flboot $(LIBNAME) -L$(LIB) -lc ; \
	else \
		$(LD) $(LDFLAG) -M olboot.map misc.o flboot.o loadprog.o \
			-o flboot $(LIBNAME) -L$(CCSLIB) -dn -lc ; \
	fi

$(LIBNAME): $(LFILES)

clean:
	rm -f olboot flboot *.o

clobber: clean
	rm -f $(LIBNAME)

print:
	pr -n lboot.c lbld lboot.dis lboot.name lboot.size | \
		opr -f hole -txr -p land

debug: olboot
	$(SIZE) olboot > olboot.size
	$(DIS) -L olboot > olboot.dis
	$(NM) -nefx olboot > olboot.name
	$(STRIP) olboot

FRC:


#
# Header dependencies
#

lboot.o: lboot.c \
	$(UINC)/a.out.h \
	$(UINC)/aouthdr.h \
	$(UINC)/filehdr.h \
	$(UINC)/linenum.h \
	$(UINC)/nlist.h \
	$(UINC)/reloc.h \
	$(UINC)/scnhdr.h \
	$(UINC)/storclass.h \
	$(UINC)/syms.h \
	$(INCSYS)/sys/boot.h \
	$(INCSYS)/sys/csr.h \
	$(INCSYS)/sys/elog.h \
	$(INCSYS)/sys/firmware.h \
	$(INCSYS)/sys/id.h \
	$(INCSYS)/sys/immu.h \
	$(INCLOC)/sys/inode.h \
	$(INCLOC)/sys/iobuf.h \
	$(INCSYS)/sys/lboot.h \
	$(INCSYS)/sys/nvram.h \
	$(INCLOC)/sys/param.h \
	$(INCSYS)/sys/sbd.h \
	$(INCSYS)/sys/types.h \
	$(INCSYS)/sys/psw.h \
	$(FRC)

flboot.o: flboot.c \
	$(UINC)/a.out.h \
	$(UINC)/aouthdr.h \
	$(UINC)/filehdr.h \
	$(UINC)/linenum.h \
	$(UINC)/nlist.h \
	$(UINC)/reloc.h \
	$(UINC)/scnhdr.h \
	$(UINC)/storclass.h \
	$(UINC)/syms.h \
	$(INCSYS)/sys/boot.h \
	$(INCSYS)/sys/csr.h \
	$(INCSYS)/sys/elog.h \
	$(INCSYS)/sys/firmware.h \
	$(INCSYS)/sys/id.h \
	$(INCSYS)/sys/immu.h \
	$(INCLOC)/sys/inode.h \
	$(INCLOC)/sys/iobuf.h \
	$(INCSYS)/sys/lboot.h \
	$(INCSYS)/sys/nvram.h \
	$(INCLOC)/sys/param.h \
	$(INCSYS)/sys/sbd.h \
	$(INCSYS)/sys/types.h \
	$(INCSYS)/sys/psw.h \
	$(FRC)

$(LIBNAME)(basicio.o): basicio.c \
	$(INCSYS)/sys/boot.h \
	$(INCSYS)/sys/elog.h \
	$(INCSYS)/sys/firmware.h \
	$(INCSYS)/sys/ino.h \
	$(INCLOC)/sys/inode.h \
	$(INCLOC)/sys/iobuf.h \
	$(INCSYS)/sys/lboot.h \
	$(INCLOC)/sys/param.h \
	$(INCSYS)/sys/sysmacros.h \
	$(INCSYS)/sys/types.h \
	$(INCSYS)/sys/sbd.h \
	$(INCSYS)/sys/psw.h \
	$(INCSYS)/sys/immu.h \
	$(INCSYS)/sys/nvram.h \
	$(FRC)

$(LIBNAME)(findfile.o): findfile.c \
	$(INCLOC)/sys/dir.h \
	$(INCSYS)/sys/firmware.h \
	$(INCLOC)/sys/inode.h \
	$(INCSYS)/sys/lboot.h \
	$(INCLOC)/sys/param.h \
	$(INCSYS)/sys/types.h \
	$(FRC)

$(LIBNAME)(findfs.o): findfs.c \
	$(INCSYS)/sys/boot.h \
	$(INCSYS)/sys/elog.h \
	$(INCLOC)/sys/filsys.h \
	$(INCSYS)/sys/firmware.h \
	$(INCLOC)/sys/inode.h \
	$(INCLOC)/sys/iobuf.h \
	$(INCSYS)/sys/lboot.h \
	$(INCLOC)/sys/param.h \
	$(INCSYS)/sys/types.h \
	$(FRC)

$(LIBNAME)(loadfile.o): loadfile.c \
	$(UINC)/a.out.h \
	$(UINC)/aouthdr.h \
	$(UINC)/filehdr.h \
	$(UINC)/linenum.h \
	$(UINC)/nlist.h \
	$(UINC)/reloc.h \
	$(UINC)/scnhdr.h \
	$(UINC)/storclass.h \
	$(UINC)/syms.h \
	$(INCSYS)/sys/firmware.h \
	$(INCLOC)/sys/inode.h \
	$(INCSYS)/sys/lboot.h \
	$(INCLOC)/sys/param.h \
	$(INCSYS)/sys/types.h \
	$(FRC)

$(LIBNAME)(oloadp.o): oloadp.c \
	$(UINC)/a.out.h \
	$(UINC)/aouthdr.h \
	$(UINC)/filehdr.h \
	$(UINC)/linenum.h \
	$(UINC)/nlist.h \
	$(UINC)/reloc.h \
	$(UINC)/scnhdr.h \
	$(UINC)/storclass.h \
	$(UINC)/syms.h \
	$(INCSYS)/sys/firmware.h \
	$(INCLOC)/sys/inode.h \
	$(INCSYS)/sys/lboot.h \
	$(INCLOC)/sys/param.h \
	$(INCSYS)/sys/types.h \
	$(FRC)

loadprog.o: loadprog.c \
	$(UINC)/a.out.h \
	$(UINC)/aouthdr.h \
	$(UINC)/filehdr.h \
	$(UINC)/linenum.h \
	$(UINC)/nlist.h \
	$(UINC)/reloc.h \
	$(UINC)/scnhdr.h \
	$(UINC)/storclass.h \
	$(UINC)/syms.h \
	$(INCSYS)/sys/firmware.h \
	$(INCLOC)/sys/inode.h \
	$(INCSYS)/sys/lboot.h \
	$(INCLOC)/sys/param.h \
	$(INCSYS)/sys/types.h \
	$(FRC)

misc.o: misc.s \
	$(FRC)

