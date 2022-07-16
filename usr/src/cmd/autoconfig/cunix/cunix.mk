#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)autoconfig:cunix/cunix.mk	1.15"

ROOT=
DIR=$(ROOT)/usr/sbin
INC=$(ROOT)/usr/include
LINC=..
#used in special situations should be INC in cross environment
SINC=$(INC)
INC_LIST	=\
	-I$(LINC)\
	-I$(INC)\
	-I$(SINC)

INS=	install
LINT=	lint -n
STRIP=	strip

#TEST=	-DTEST \
	-DDEBUG1 

CPPOPT=	 $(INC_LIST) $(TEST) -Du3b2
DASHO= -O
CFLAGS=	 $(DASHO) $(CPPOPT)
LDFLAGS=
LBLD=
FRC=

all: cunix 

OBJECT=	error.o driver_proc.o edt.o gen.o int_error.o machdep.o main.o off.o parse.o sym.o cmisc.o elf_func.o xmalloc.o

cunix:	$(OBJECT) $(FRC)
	$(CC) $(LDFLAGS) $(OBJECT) -lelf -o cunix $(NOSHLIBS)



install: all
	$(STRIP) cunix
	$(INS) -f $(DIR) -m 0555 -u root -g root cunix

clean:
	rm -f *.o ../sys/error.h

clobber: clean
	rm -f cunix

FRC:

../sys/error.h:errortable
	-echo '/^[ \t]*$$/d' >sed$$$$; \
	echo '/^#/d' >>sed$$$$; \
	echo 's/^[ \t]*//' >>sed$$$$; \
	echo 's/[ \t].*//' >>sed$$$$; \
	echo 's/^/__DEF__\t/' >>sed$$$$; \
	echo 's/$$/\t__LINE__/' >>sed$$$$; \
	echo '#define __DEF__ #define\n' >temp$$$$; \
	$(CC) -E $(CPPOPT) errortable | sed -f sed$$$$ >>temp$$$$; \
	$(CC) -E temp$$$$ >../sys/error.h; \
	rm -f temp$$$$ sed$$$$

error.c:../sys/error.h
	-echo '#include <sys/types.h>' >error.c; \
	echo '#include <sys/int_error.h>' >>error.c; \
	echo '#include <sys/error.h>' >>error.c; \
	echo '\nstruct errortab errortab[] = {' >>error.c; \
	echo '/^[ \t]*$$/d' >sed$$$$; \
	echo '/^#/d' >>sed$$$$; \
	echo 's/^[ \t]*//' >>sed$$$$; \
	echo 't reset' >>sed$$$$; \
	echo ': reset' >>sed$$$$; \
	echo 's/^\\([^ \t]*\\)[ \t][ \t]*\\([^ \t]*\\)[ \t][ \t]*\\(".*[^\\\\]"\\)/\t{ \\1, \\2, \\3 },/p' >>sed$$$$; \
	echo 't done' >>sed$$$$; \
	echo 's/^\\([^ \t]*\\)[ \t][ \t]*\\([^ \t]*\\)/\t{ \\1, \\2, NULL },/p' >>sed$$$$; \
	echo ': done' >>sed$$$$; \
	$(CC) -E $(CPPOPT) errortable | sed -f sed$$$$ >>error.c; \
	rm -f sed$$$$; \
	echo '\t{ 0 } };' >>error.c


#
# Header dependencies
#

xmalloc.o: xmalloc.c \
	$(SINC)/stdio.h\
	$(SINC)/fcntl.h\
	$(INC)/sys/mman.h\
	$(FRC)

elf_func.o: elf_func.c \
	$(INC)/sys/types.h \
	$(LINC)/sys/localtypes.h \
	$(SINC)/stdio.h \
	$(SINC)/a.out.h \
	$(INC)/sys/param.h \
	$(INC)/sys/dir.h \
	$(SINC)/dirent.h \
	$(INC)/sys/sys3b.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/conf.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/boothdr.h \
	$(LINC)/sys/error.h \
	$(LINC)/sys/ledt.h \
	$(LINC)/sys/sym.h \
	$(LINC)/sys/dproc.h \
	$(LINC)/sys/machdep.h \
	$(LINC)/sys/gen.h \
	$(LINC)/sys/off.h \
	$(INC)/sys/fcntl.h \
	$(LINC)/sys/cunix.h \
	$(FRC)


driver_proc.o: driver_proc.c \
	$(INC)/sys/types.h \
	$(LINC)/sys/localtypes.h \
	$(SINC)/stdio.h \
	$(SINC)/a.out.h \
	$(INC)/sys/param.h \
	$(INC)/sys/conf.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/boothdr.h \
	$(INC)/sys/sys3b.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(LINC)/sys/error.h \
	$(LINC)/sys/sym.h \
	$(LINC)/sys/dproc.h \
	$(LINC)/sys/machdep.h \
	$(LINC)/sys/gen.h \
	$(LINC)/sys/off.h \
	$(INC)/sys/fcntl.h \
	$(LINC)/sys/cunix.h \
	$(FRC)


edt.o: edt.c \
	$(INC)/sys/types.h \
	$(LINC)/sys/localtypes.h \
	$(SINC)/stdio.h \
	$(SINC)/a.out.h \
	$(INC)/sys/boothdr.h \
	$(INC)/sys/sys3b.h \
	$(INC)/sys/edt.h \
	$(INC)/sys/extbus.h \
	$(LINC)/sys/error.h \
	$(LINC)/sys/dproc.h \
	$(LINC)/sys/gen.h \
	$(LINC)/sys/ledt.h \
	$(INC)/sys/vtoc.h \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/sbd.h \
	$(LINC)/sys/cunix.h \
	$(FRC)


gen.o: gen.c \
	$(INC)/sys/types.h \
	$(LINC)/sys/localtypes.h \
	$(SINC)/stdio.h \
	$(SINC)/a.out.h \
	$(INC)/sys/param.h \
	$(INC)/sys/dir.h \
	$(SINC)/dirent.h \
	$(INC)/sys/sys3b.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/conf.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/boothdr.h \
	$(LINC)/sys/error.h \
	$(LINC)/sys/ledt.h \
	$(LINC)/sys/sym.h \
	$(LINC)/sys/dproc.h \
	$(LINC)/sys/machdep.h \
	$(LINC)/sys/gen.h \
	$(LINC)/sys/off.h \
	$(INC)/sys/fcntl.h \
	$(LINC)/sys/cunix.h \
	$(FRC)


int_error.o: int_error.c \
	$(INC)/sys/types.h \
	$(SINC)/stdio.h \
	$(LINC)/sys/localtypes.h \
	$(LINC)/sys/int_error.h \
	$(LINC)/sys/error.h \
	$(FRC)


machdep.o: machdep.c \
	$(INC)/sys/types.h \
	$(LINC)/sys/localtypes.h \
	$(SINC)/stdio.h \
	$(SINC)/a.out.h \
	$(INC)/sys/param.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/boothdr.h \
	$(INC)/sys/sys3b.h \
	$(INC)/sys/edt.h \
	$(INC)/sys/extbus.h \
	$(LINC)/sys/error.h \
	$(LINC)/sys/dproc.h \
	$(LINC)/sys/machdep.h \
	$(LINC)/sys/gen.h \
	$(LINC)/sys/off.h \
	$(LINC)/sys/ledt.h \
	$(INC)/sys/vtoc.h \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/iobd.h \
	$(LINC)/sys/cunix.h \
	$(FRC)


main.o: main.c \
	$(INC)/sys/types.h \
	$(LINC)/sys/localtypes.h \
	$(SINC)/stdio.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/boothdr.h \
	$(LINC)/sys/error.h \
	$(FRC) 

off.o: off.c \
	$(SINC)/string.h \
	$(INC)/sys/types.h \
	$(LINC)/sys/localtypes.h \
	$(SINC)/time.h \
	$(SINC)/stdio.h \
	$(INC)/sys/fcntl.h \
	$(SINC)/a.out.h \
	$(INC)/sys/sys3b.h \
	$(LINC)/sys/ledt.h \
	$(LINC)/sys/dproc.h \
	$(LINC)/sys/gen.h \
	$(LINC)/sys/off.h \
	$(LINC)/sys/sym.h \
	$(LINC)/sys/error.h \
	$(LINC)/sys/cunix.h \
	$(FRC)


parse.o: parse.c \
	$(INC)/sys/types.h \
	$(LINC)/sys/localtypes.h \
	$(SINC)/stdio.h \
	$(SINC)/a.out.h \
	$(INC)/sys/sys3b.h \
	$(INC)/sys/dir.h \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/sysmacros.h \
	$(LINC)/sys/dproc.h \
	$(LINC)/sys/machdep.h \
	$(SINC)/ctype.h \
	$(LINC)/sys/error.h \
	$(LINC)/sys/cunix.h \
	$(FRC)


sym.o: sym.c \
	$(SINC)/string.h \
	$(INC)/sys/types.h \
	$(LINC)/sys/localtypes.h \
	$(SINC)/stdio.h \
	$(SINC)/a.out.h \
	$(INC)/sys/sys3b.h \
	$(LINC)/sys/off.h \
	$(LINC)/sys/sym.h \
	$(LINC)/sys/error.h \
	$(LINC)/sys/cunix.h \
	$(FRC)

cmisc.o: cmisc.s \
	$(FRC)

