#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)boot:boot/filledt/dcp/dcp.mk	1.13"

ROOT=
DIR=/$(ROOT)
INC=$(ROOT)/usr/include
UINC=$(ROOT)/usr/include
LINC=../com
SINC=../..
INC_LIST	=\
	-I$(LINC)\
	-I$(SINC)\
	-I$(INC)\
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

DG_OBJ = dcpcntl.o dgn.o dgnerror.o dumpsoak.o execphsz.o \
	getdgn.o getnum.o initsoak.o parse.o ph_check.o \
	ph_list.o setup.o soak.o soakrec.o p_func.o fw_pump.o

filledt: ifile filledt.map $(FL_OBJ)
	if [ x$(CCSTYPE) = xCOFF ] ; \
	then \
		$(LD) $(LDFLAGS) -o filledt $(FL_OBJ) ifile $(LIBFM) ; \
	else \
		$(LD) $(LDFLAGS) -o filledt $(FL_OBJ) -M filledt.map $(LIBFM) -dn ; \
	fi

dgmon:	ifile.dgmon dgmon.map data.o dgdata.o $(DG_OBJ) $(LIBFM)
	if [ x$(CCSTYPE) = xCOFF ] ; \
	then \
		$(LD) $(LDFLAGS) -o dgmon $(DG_OBJ) dgdata.o ifile.dgmon $(LIBFM) ; \
	else \
		$(LD) $(LDFLAGS) -o dgmon $(DG_OBJ) data.o -M dgmon.map  $(LIBFM) -dn ; \
	fi

data.o: data.s
	if [ x$(CCSTYPE) != xCOFF ] ; \
	then \
		$(AS) data.s ; \
	fi

dgdata.o: dgdata.s
	if [ x$(CCSTYPE) = xCOFF ] ; \
	then \
		$(AS) dgdata.s ; \
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
	$(INC)/sys/boot.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/csr.h \
	$(INC)/sys/firmware.h \
	$(INC)/sys/iu.h \
	$(INC)/sys/sit.h \
	$(INC)/sys/edt.h \
	$(INC)/sys/iobd.h \
	$(INC)/sys/diagnostic.h \
	$(INC)/sys/dsd.h \
	$(LINC)/edt_def.h \
	$(FRC)

edt_fill.o: edt_fill.c \
	$(INC)/sys/types.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/diagnostic.h \
	$(INC)/sys/edt.h \
	$(INC)/sys/firmware.h \
	$(LINC)/edt_def.h \
	$(INC)/sys/extbus.h \
	$(INC)/sys/boot.h \
	$(SINC)/sys/inode.h \
	$(INC)/sys/lboot.h \
	$(INC)/sys/dsd.h \
	$(INC)/sys/vtoc.h \
	$(FRC)

fillerror.o: fillerror.c \
	$(INC)/sys/firmware.h \
	$(INC)/sys/edt.h \
	$(INC)/sys/diagnostic.h \
	$(FRC)

loc_cons.o: loc_cons.c \
	$(INC)/sys/sbd.h \
	$(INC)/sys/iu.h \
	$(INC)/sys/firmware.h \
	$(INC)/sys/nvram.h \
	$(INC)/sys/edt.h \
	$(INC)/sys/dsd.h \
	$(INC)/sys/iobd.h \
	$(INC)/sys/diagnostic.h \
        $(INC)/sys/termio.h \
	$(FRC)

setup.o: setup.c \
	$(INC)/sys/types.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/boot.h \
	$(INC)/sys/firmware.h \
	$(INC)/sys/diagnostic.h \
	$(SINC)/sys/inode.h \
	$(SINC)/sys/param.h \
	$(INC)/sys/extbus.h \
	$(INC)/sys/lboot.h \
	$(INC)/sys/id.h \
	$(INC)/sys/vtoc.h \
	$(FRC)

syg.o: syg.c \
	$(INC)/sys/sbd.h \
	$(INC)/sys/firmware.h \
	$(INC)/sys/edt.h \
	$(INC)/sys/iobd.h \
	$(INC)/sys/diagnostic.h \
        $(LINC)/edt_def.h \
	$(INC)/sys/dsd.h \
	$(INC)/sys/boot.h \
	$(FRC)

fw_pump.o: fw_pump.c \
	$(INC)/sys/types.h \
	$(SINC)/sys/inode.h \
	$(INC)/a.out.h \
	$(INC)/sys/firmware.h \
	$(SINC)/sys/param.h \
	$(INC)/sys/lboot.h \
	$(INC)/sys/boot.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/csr.h \
	$(INC)/sys/cio_defs.h \
	$(INC)/sys/diagnostic.h \
	$(INC)/sys/iobd.h \
	$(FRC)

p_func.o: p_func.c \
	$(INC)/sys/sbd.h \
	$(INC)/sys/firmware.h \
	$(INC)/sys/edt.h \
	$(INC)/sys/iobd.h \
	$(INC)/sys/diagnostic.h \
	$(INC)/sys/dsd.h \
	$(INC)/sys/boot.h \
	$(INC)/sys/csr.h \
	$(INC)/sys/iu.h \
	$(INC)/sys/sit.h \
	$(INC)/sys/cio_defs.h \
	$(FRC)

