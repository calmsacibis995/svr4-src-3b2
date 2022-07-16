#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)debug:debug/debug.mk	1.7"

M4=m4 m4.defs
CC=cc
DASHO = -O
CFLAGS= -I$(INC) -D_KERNEL $(MORECPP) $(DASHO)
PROF=
DEFLIST=
INC=$(ROOT)/usr/include
MKBOOT = mkboot
MASTERD = ../master.d
FRC=

OBJECTS=\
	arg.o \
	ctype.o \
	data.o \
	disasm.o \
	doprnt.o \
	prtabs.o \
	sprintf.o \
	tables.o \
	trace.o \
	utils.o

all:	DEBUG

DEBUG: debug.o $(MASTERD)/debug
	$(MKBOOT) -m $(MASTERD) -d . debug.o;

debug.o: $(OBJECTS)
	$(LD) -r -o debug.o $(OBJECTS)

.c.o:
	$(CC) $(DEFLIST) $(CFLAGS) -c $*.c 

.s.o:
	$(M4) -DMCOUNT=#  $*.s   > $*.m.s
	$(CC) $(DEFLIST) $(CFLAGS) -c $*.m.s
	mv $*.m.o $*.o
	rm $*.m.s
clean:
	-rm -f *.o

clobber: clean
	-rm -f debug DEBUG

FRC:

#
# Header dependencies
#

arg.o: arg.s \
	m4.defs \
	$(FRC)

ctype.o: ctype.c \
	ctype.h \
	$(FRC)

data.o: data.c \
	stdio.h \
	$(FRC)

disasm.o: disasm.c \
	$(INC)/sys/types.h \
	$(INC)/sys/inline.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/sys3b.h \
	$(INC)/sys/disasm.h \
	$(INC)/sys/sgs.h \
	$(FRC)

doprnt.o: doprnt.s \
	m4.defs \
	print.defs \
	$(FRC)

prtabs.o: prtabs.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/fs/s5dir.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/user.h \
	$(INC)/sys/var.h \
	$(INC)/sys/evecb.h \
	$(INC)/sys/hrtcntl.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/hrtsys.h \
	$(INC)/sys/inline.h \
	$(INC)/sys/immu.h \
	$(INC)/vm/vm_hat.h \
	$(INC)/vm/as.h \
	$(INC)/vm/page.h \
	$(INC)/vm/seg.h \
	$(INC)/vm/hat.h \
	$(INC)/vm/seg_vn.h \
	$(INC)/vm/anon.h \
	$(FRC)

sprintf.o: sprintf.s \
	m4.defs \
	print.defs \
	$(FRC)

tables.o: tables.c \
	$(INC)/sys/types.h \
	$(INC)/sys/disasm.h \
	$(FRC)

trace.o: trace.c \
	$(INC)/sys/types.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/param.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/fs/s5dir.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/user.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/sys3b.h \
	$(INC)/sys/inline.h \
	$(INC)/sys/var.h \
	$(INC)/sys/immu.h \
	$(INC)/vm/as.h \
	$(INC)/vm/vm_hat.h \
	$(FRC)

utils.o: utils.c \
	$(INC)/sys/types.h \
	$(INC)/sys/inline.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/param.h \
	$(INC)/sys/conf.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/sys3b.h \
	$(INC)/sys/disasm.h \
	$(INC)/sys/sgs.h \
	$(FRC)

