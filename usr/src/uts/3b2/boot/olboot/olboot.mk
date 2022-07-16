#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)boot:boot/olboot/olboot.mk	11.17"

ROOT =
INC = $(ROOT)/usr/include
UINC = $(ROOT)/usr/include
INCLOC = ..
DASHO = -O
CFLAGS = $(DASHO) -I$(INCLOC) -I$(INC) -I$(UINC) $(DBO)
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
	../boot/strncat.o\
	$(LIBNAME)\
	$(FRC)
	if [ x$(CCSTYPE) = xCOFF ] ; \
	then \
		$(LD) $(LDFLAGS) lbld misc.o lboot.o loadprog.o \
			../boot/strncat.o -o olboot $(LIBNAME) ; \
	else \
		$(LD) $(LDFLAGS) -M olboot.map misc.o lboot.o loadprog.o \
			../boot/strncat.o -o olboot $(LIBNAME) -dn ; \
	fi

flboot: lbld\
	olboot.map\
	misc.o\
	flboot.o\
	loadprog.o\
	../boot/strncat.o\
	$(LIBNAME)\
	$(FRC)
	if [ x$(CCSTYPE) = xCOFF ] ; \
	then \
		$(LD) $(LDFLAGS) lbld misc.o flboot.o loadprog.o \
			../boot/strncat.o -o flboot $(LIBNAME) ; \
	else \
		$(LD) $(LDFLAGS) -M olboot.map misc.o flboot.o loadprog.o \
			../boot/strncat.o -o flboot $(LIBNAME) -dn ; \
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
	$(INC)/a.out.h \
	$(INC)/aouthdr.h \
	$(INC)/filehdr.h \
	$(INC)/linenum.h \
	$(INC)/nlist.h \
	$(INC)/reloc.h \
	$(INC)/scnhdr.h \
	$(INC)/storclass.h \
	$(INC)/syms.h \
	$(INC)/sys/boot.h \
	$(INC)/sys/csr.h \
	$(INC)/sys/elog.h \
	$(INC)/sys/firmware.h \
	$(INC)/sys/id.h \
	$(INC)/sys/immu.h \
	$(INCLOC)/sys/inode.h \
	$(INCLOC)/sys/iobuf.h \
	$(INC)/sys/lboot.h \
	$(INC)/sys/nvram.h \
	$(INCLOC)/sys/param.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/types.h \
	$(INC)/sys/psw.h \
	$(FRC)

flboot.o: flboot.c \
	$(INC)/a.out.h \
	$(INC)/aouthdr.h \
	$(INC)/filehdr.h \
	$(INC)/linenum.h \
	$(INC)/nlist.h \
	$(INC)/reloc.h \
	$(INC)/scnhdr.h \
	$(INC)/storclass.h \
	$(INC)/syms.h \
	$(INC)/sys/boot.h \
	$(INC)/sys/csr.h \
	$(INC)/sys/elog.h \
	$(INC)/sys/firmware.h \
	$(INC)/sys/id.h \
	$(INC)/sys/immu.h \
	$(INCLOC)/sys/inode.h \
	$(INCLOC)/sys/iobuf.h \
	$(INC)/sys/lboot.h \
	$(INC)/sys/nvram.h \
	$(INCLOC)/sys/param.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/types.h \
	$(INC)/sys/psw.h \
	$(FRC)

$(LIBNAME)(basicio.o): basicio.c \
	$(INC)/sys/boot.h \
	$(INC)/sys/elog.h \
	$(INC)/sys/firmware.h \
	$(INC)/sys/ino.h \
	$(INCLOC)/sys/inode.h \
	$(INCLOC)/sys/iobuf.h \
	$(INC)/sys/lboot.h \
	$(INCLOC)/sys/param.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/types.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/nvram.h \
	$(FRC)

$(LIBNAME)(findfile.o): findfile.c \
	$(INCLOC)/sys/dir.h \
	$(INC)/sys/firmware.h \
	$(INCLOC)/sys/inode.h \
	$(INC)/sys/lboot.h \
	$(INCLOC)/sys/param.h \
	$(INC)/sys/types.h \
	$(FRC)

$(LIBNAME)(findfs.o): findfs.c \
	$(INC)/sys/boot.h \
	$(INC)/sys/elog.h \
	$(INCLOC)/sys/filsys.h \
	$(INC)/sys/firmware.h \
	$(INCLOC)/sys/inode.h \
	$(INCLOC)/sys/iobuf.h \
	$(INC)/sys/lboot.h \
	$(INCLOC)/sys/param.h \
	$(INC)/sys/types.h \
	$(FRC)

$(LIBNAME)(loadfile.o): loadfile.c \
	$(INC)/a.out.h \
	$(INC)/aouthdr.h \
	$(INC)/filehdr.h \
	$(INC)/linenum.h \
	$(INC)/nlist.h \
	$(INC)/reloc.h \
	$(INC)/scnhdr.h \
	$(INC)/storclass.h \
	$(INC)/syms.h \
	$(INC)/sys/firmware.h \
	$(INCLOC)/sys/inode.h \
	$(INC)/sys/lboot.h \
	$(INCLOC)/sys/param.h \
	$(INC)/sys/types.h \
	$(FRC)

$(LIBNAME)(oloadp.o): oloadp.c \
	$(INC)/a.out.h \
	$(INC)/aouthdr.h \
	$(INC)/filehdr.h \
	$(INC)/linenum.h \
	$(INC)/nlist.h \
	$(INC)/reloc.h \
	$(INC)/scnhdr.h \
	$(INC)/storclass.h \
	$(INC)/syms.h \
	$(INC)/sys/firmware.h \
	$(INCLOC)/sys/inode.h \
	$(INC)/sys/lboot.h \
	$(INCLOC)/sys/param.h \
	$(INC)/sys/types.h \
	$(FRC)

loadprog.o: loadprog.c \
	$(INC)/a.out.h \
	$(INC)/aouthdr.h \
	$(INC)/filehdr.h \
	$(INC)/linenum.h \
	$(INC)/nlist.h \
	$(INC)/reloc.h \
	$(INC)/scnhdr.h \
	$(INC)/storclass.h \
	$(INC)/syms.h \
	$(INC)/sys/firmware.h \
	$(INCLOC)/sys/inode.h \
	$(INC)/sys/lboot.h \
	$(INCLOC)/sys/param.h \
	$(INC)/sys/types.h \
	$(FRC)

misc.o: misc.s \
	$(FRC)

