#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kernel:ml/ml.mk	1.8"

ROOT =
INC=$(ROOT)/usr/include
DASHO = -O
CFLAGS= $(DASHO) -I$(INC) -D_KERNEL $(MORECPP)
MKBOOT = mkboot
MASTERD = ../master.d
FRC =

SFILES = ttrap.s cswitch.s misc.s string.s

all:	../locore.o ../start.o ../gate.o ../syms.o

../locore.o:$(SFILES) $(FRC) kpcbs.o
	cat $(SFILES) | \
		sed -e 's/^#/?/' \
			-e 's/?\([ 	]*ifdef.*\)/#\1/' \
			-e 's/?\([ 	]*ifndef.*\)/#\1/' \
			-e 's/?\([ 	]*else.*\)/#\1/' \
			-e 's/?\([ 	]*endif.*\)/#\1/' \
			-e '/^?/d' > TMP$$$$.c && \
		$(CC) -P $(MORECPP) TMP$$$$.c && \
		mv TMP$$$$.i locore.s && \
	$(CC) $(DASHO) -c locore.s && \
	$(LD) -r -o ../locore.o locore.o kpcbs.o && \
	rm -f locore.o locore.s TMP$$$$.c

../start.o:$(FRC) uprt.o pstart.o cdump.o
	$(LD)  -r -o ../start.o uprt.o pstart.o cdump.o ; \
	if [ x$(CCSTYPE) != xCOFF ] ; \
	then \
		$(MKBOOT) -m $(MASTERD) -d .. ../start.o ; \
	fi

../gate.o: gate.c \
	$(INC)/sys/types.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/gate.h \
	$(INC)/sys/pcb.h \
	$(FRC)
	$(CC) $(CFLAGS) -c gate.c
	mv gate.o ../gate.o
	if [ x$(CCSTYPE) != xCOFF ] ; \
	then $(MKBOOT) -m $(MASTERD) -d .. ../gate.o ; \
	fi

../syms.o: $(FRC) syms.s
	if [ x$(CCSTYPE) != xCOFF ] ; \
	then \
		$(CC) $(CFLAGS) -c syms.s && \
		mv syms.o ../syms.o ; \
	else \
		touch ../syms.o ; \
	fi


uprt.o:uprt.s $(FRC)
	cat uprt.s | \
		sed -e 's/^#/?/' \
			-e 's/?\([ 	]*ifdef.*\)/#\1/' \
			-e 's/?\([ 	]*ifndef.*\)/#\1/' \
			-e 's/?\([ 	]*else.*\)/#\1/' \
			-e 's/?\([ 	]*endif.*\)/#\1/' \
			-e '/^?/d' > TMP$$$$.c && \
		$(CC) -P TMP$$$$.c && \
		mv TMP$$$$.i TMP$$$$.s && \
		$(CC) $(DASHO) -c TMP$$$$.s && \
		mv TMP$$$$.o uprt.o && \
		rm -f TMP$$$$.?

clean:
	-rm -f a.out *.o TMP*.?

clobber:	clean
	-rm -f ../locore.o ../start.o ../gate.o ../syms.o ../GATE ../START

FRC:

#
# Header dependencies
#

cdump.o: cdump.c \
	$(INC)/sys/sbd.h \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/nvram.h \
	$(INC)/sys/firmware.h \
	$(INC)/sys/cdump.h \
	$(INC)/sys/todc.h \
	$(INC)/sys/sysmacros.h \
	$(FRC)

kpcbs.o: kpcbs.c \
	$(INC)/sys/types.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/param.h \
	$(FRC)

pstart.o: pstart.c \
	$(INC)/sys/types.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/param.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/csr.h \
	$(INC)/sys/sit.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/fs/s5dir.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/user.h \
	$(INC)/sys/firmware.h \
	$(INC)/sys/nvram.h \
	$(INC)/sys/var.h \
	$(INC)/sys/iobd.h \
	$(FRC)
