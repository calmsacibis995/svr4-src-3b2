#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)boot:boot/filledt/dcp/dcp.mk	1.9"

ROOT=
DIR=$(ROOT)
LIB=$(ROOT)/lib
INCSYS=$(ROOT)/usr/include
UINC=$(ROOT)/usr/include
LINC=../com
SINC=../..
INC_LIST	=\
	-I$(LINC)\
	-I$(SINC)\
	-I$(INCSYS)\
	-I$(UINC)


INS=	install
LINT=	lint -n
STRIP=	strip

#TEST=	-DTEST \
	-DDEBUG1 

CPPOPT=	 $(INC_LIST)  -UDEBUG -Dm32 -Dfw3b2
DASHO= -O
CFLAGS=	 $(DASHO) $(CPPOPT)
LIBFM=../../olboot/libfm.a
LDFLAGS=
LBLD=
FRC=

all: filledt dgmon

FL_OBJ = fillcntl.o edt_fill.o fillerror.o loc_cons.o setup.o syg.o \
	fw_pump.o p_func.o

DG_OBJ = data.o dcpcntl.o dgn.o dgnerror.o dumpsoak.o execphsz.o \
	getdgn.o getnum.o initsoak.o parse.o ph_check.o \
	ph_list.o setup.o soak.o soakrec.o p_func.o fw_pump.o

filledt: ifile filledt.map $(FL_OBJ)
	if [ x$(CCSTYPE) = xCOFF ] ; \
	then \
		$(LD) -o filledt $(FL_OBJ) ifile $(LIBFM) -L$(LIB) -lc ; \
	else \
		$(LD) -o filledt $(FL_OBJ) -M filledt.map $(LIBFM) -L$(CCSLIB) -dn -lc ; \
	fi

dgmon:	ifile dgmon.map $(DG_OBJ) $(LIBFM)
	if [ x$(CCSTYPE) = xCOFF ] ; \
	then \
		$(LD) -o dgmon $(DG_OBJ) ifile $(LIBFM) -L$(LIB) -lc ; \
	else \
		$(LD) -o dgmon $(DG_OBJ) -M dgmon.map  $(LIBFM) -L$(LIB) -dn -lc ; \
	fi

install: all
	$(STRIP) filledt
	$(INS) -f $(DIR) filledt
	$(STRIP) dgmon
	$(INS) -f $(ROOT)/init dgmon

clean:
	rm -f *.o 

clobber: clean
	rm -f filledt dgmon

FRC:

fillcntl.o: fillcntl.c \
	$(INCSYS)/sys/boot.h \
	$(INCSYS)/sys/sbd.h \
	$(INCSYS)/sys/csr.h \
	$(INCSYS)/sys/firmware.h \
	$(INCSYS)/sys/iu.h \
	$(INCSYS)/sys/sit.h \
	$(INCSYS)/sys/edt.h \
	$(INCSYS)/sys/iobd.h \
	$(INCSYS)/sys/diagnostic.h \
	$(INCSYS)/sys/dsd.h \
	$(LINC)/edt_def.h \
	$(FRC)

edt_fill.o: edt_fill.c \
	$(INCSYS)/sys/types.h \
	$(INCSYS)/sys/sbd.h \
	$(INCSYS)/sys/diagnostic.h \
	$(INCSYS)/sys/edt.h \
	$(INCSYS)/sys/firmware.h \
	$(LINC)/edt_def.h \
	$(INCSYS)/sys/extbus.h \
	$(INCSYS)/sys/boot.h \
	$(SINC)/sys/inode.h \
	$(INCSYS)/sys/lboot.h \
	$(INCSYS)/sys/dsd.h \
	$(INCSYS)/sys/vtoc.h \
	$(FRC)

fillerror.o: fillerror.c \
	$(INCSYS)/sys/firmware.h \
	$(INCSYS)/sys/edt.h \
	$(INCSYS)/sys/diagnostic.h \
	$(FRC)

loc_cons.o: loc_cons.c \
	$(INCSYS)/sys/sbd.h \
	$(INCSYS)/sys/iu.h \
	$(INCSYS)/sys/firmware.h \
	$(INCSYS)/sys/nvram.h \
	$(INCSYS)/sys/edt.h \
	$(INCSYS)/sys/dsd.h \
	$(INCSYS)/sys/iobd.h \
	$(INCSYS)/sys/diagnostic.h \
        $(INCSYS)/sys/termio.h \
	$(FRC)

setup.o: setup.c \
	$(INCSYS)/sys/types.h \
	$(INCSYS)/sys/sbd.h \
	$(INCSYS)/sys/boot.h \
	$(INCSYS)/sys/firmware.h \
	$(INCSYS)/sys/diagnostic.h \
	$(SINC)/sys/inode.h \
	$(SINC)/sys/param.h \
	$(INCSYS)/sys/extbus.h \
	$(INCSYS)/sys/lboot.h \
	$(INCSYS)/sys/id.h \
	$(INCSYS)/sys/vtoc.h \
	$(FRC)

syg.o: syg.c \
	$(INCSYS)/sys/sbd.h \
	$(INCSYS)/sys/firmware.h \
	$(INCSYS)/sys/edt.h \
	$(INCSYS)/sys/iobd.h \
	$(INCSYS)/sys/diagnostic.h \
        $(LINC)/edt_def.h \
	$(INCSYS)/sys/dsd.h \
	$(INCSYS)/sys/boot.h \
	$(FRC)

fw_pump.o: fw_pump.c \
	$(INCSYS)/sys/types.h \
	$(SINC)/sys/inode.h \
	$(UINC)/a.out.h \
	$(INCSYS)/sys/firmware.h \
	$(SINC)/sys/param.h \
	$(INCSYS)/sys/lboot.h \
	$(INCSYS)/sys/boot.h \
	$(INCSYS)/sys/sbd.h \
	$(INCSYS)/sys/csr.h \
	$(INCSYS)/sys/cio_defs.h \
	$(INCSYS)/sys/diagnostic.h \
	$(INCSYS)/sys/iobd.h \
	$(FRC)

p_func.o: p_func.c \
	$(INCSYS)/sys/sbd.h \
	$(INCSYS)/sys/firmware.h \
	$(INCSYS)/sys/edt.h \
	$(INCSYS)/sys/iobd.h \
	$(INCSYS)/sys/diagnostic.h \
	$(INCSYS)/sys/dsd.h \
	$(INCSYS)/sys/boot.h \
	$(INCSYS)/sys/csr.h \
	$(INCSYS)/sys/iu.h \
	$(INCSYS)/sys/sit.h \
	$(INCSYS)/sys/cio_defs.h \
	$(FRC)

