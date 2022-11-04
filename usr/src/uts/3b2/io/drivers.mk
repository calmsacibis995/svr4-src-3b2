#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kernel:io/drivers.mk	1.29"
ROOT =
STRIP = strip
INC = $(ROOT)/usr/include
MKBOOT = mkboot
MASTERD = ../master.d

DASHO = -O 
PFLAGS = -I$(INC) -D_KERNEL $(MORECPP)
CFLAGS = $(DASHO) $(PFLAGS)
FRC =

DFILES = \
	clone.o \
	gentty.o\
	hdelog.o\
	icd.o \
	idisk.o\
	iuart.o\
	log.o \
	mau.o \
	mem.o\
	prf.o\
	sad.o \
	sockmod.o \
	sp.o \
	stubs.o\
	sxt.o\
	timod.o \
	tirdwr.o

drivers:	all

all:	\
	CLONE  \
	GENTTY \
	HDELOG \
	ICD \
	IDISK \
	IUART \
	LOG  \
	MAU \
	MEM \
	PRF \
	SAD \
	SOCKMOD \
	SP  \
	STUBS \
	SXT \
	TIMOD  \
	TIRDWR

clean:
	-rm -f $(DFILES)

clobber:	clean
	-rm -f IUART SXT IDISK HDELOG LOG CLONE SP TIMOD TIRDWR
	-rm -f MEM GENTTY PRF SAD STUBS ICD MAU SOCKMOD

FRC:

CLONE:	clone.o $(MASTERD)/clone
	$(MKBOOT) -m $(MASTERD) -d . clone.o;

GENTTY:	gentty.o $(MASTERD)/gentty
	$(MKBOOT) -m $(MASTERD) -d . gentty.o; 

HDELOG:	hdelog.o $(MASTERD)/hdelog
	$(MKBOOT) -m $(MASTERD) -d . hdelog.o; 

ICD:	icd.o $(MASTERD)/icd
	$(MKBOOT) -m $(MASTERD) -d . icd.o;

IDISK:	idisk.o $(MASTERD)/idisk
	$(MKBOOT) -m $(MASTERD) -d . idisk.o; 

IUART:	iuart.o $(MASTERD)/iuart
	$(MKBOOT) -m $(MASTERD) -d . iuart.o;

LOG:	log.o $(MASTERD)/log
	$(MKBOOT) -m $(MASTERD) -d . log.o;

MAU:	mau.o $(MASTERD)/mau
	$(MKBOOT) -m $(MASTERD) -d . mau.o;

MEM:	mem.o $(MASTERD)/mem
	$(MKBOOT) -m $(MASTERD) -d . mem.o;

PRF:	prf.o $(MASTERD)/prf
	$(MKBOOT) -m $(MASTERD) -d . prf.o;

SAD:	sad.o $(MASTERD)/sad
	$(MKBOOT) -m $(MASTERD) -d . sad.o;
 
SOCKMOD:	sockmod.o $(MASTERD)/sockmod
	$(MKBOOT) -m $(MASTERD) -d . sockmod.o

SP:	sp.o $(MASTERD)/sp
	$(MKBOOT) -m $(MASTERD) -d . sp.o;

STUBS:	stubs.o $(MASTERD)/stubs
	$(MKBOOT) -m $(MASTERD) -d . stubs.o;

SXT:	sxt.o $(MASTERD)/sxt
	$(MKBOOT) -m $(MASTERD) -d . sxt.o;

TIMOD:	timod.o $(MASTERD)/timod
	$(MKBOOT) -m $(MASTERD) -d . timod.o;

TIRDWR:	tirdwr.o $(MASTERD)/tirdwr
	$(MKBOOT) -m $(MASTERD) -d . tirdwr.o;

hdelog.o: hde.o $(FRC)
	-@if [ "$(VPATH)" ]; then cp hde.o $@; else ln hde.o $@; fi

idisk.o: id.o if.o sddrv.o $(FRC)
	$(LD) -r $(LDFLAGS) -o idisk.o id.o if.o sddrv.o

#
# Header dependencies
#

clone.o: clone.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/user.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/sysinfo.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/ddi.h \
	$(INC)/sys/conf.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/inline.h \
	$(INC)/sys/mkdev.h \
	$(FRC)

gentty.o: gentty.c \
	$(INC)/sys/session.h \
	$(INC)/sys/types.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/param.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/ddi.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/user.h \
	$(INC)/sys/conf.h \
	$(INC)/sys/tty.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/uio.h \
	$(FRC)

hde.o: hde.c \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/user.h \
	$(INC)/sys/file.h \
	$(INC)/sys/conf.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/vtoc.h \
	$(INC)/sys/hdelog.h \
	$(INC)/sys/hdeioctl.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/open.h \
	$(FRC)

icd.o: icd.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/boot.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/firmware.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/user.h \
	$(INC)/sys/vtoc.h \
	$(INC)/sys/inline.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/mkdev.h \
	$(INC)/sys/ddi.h \
	$(INC)/sys/cred.h \
	$(FRC)

id.o: id.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/time.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/id.h \
	$(INC)/sys/if.h \
	$(INC)/sys/dma.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/user.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/elog.h \
	$(INC)/sys/iobuf.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/firmware.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/vtoc.h \
	$(INC)/sys/hdelog.h \
	$(INC)/sys/open.h \
	$(INC)/sys/inline.h \
	$(INC)/sys/uio.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/ddi.h \
	$(FRC)

if.o: if.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/dma.h \
	$(INC)/sys/csr.h \
	$(INC)/sys/iu.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/conf.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/user.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/elog.h \
	$(INC)/sys/iobuf.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/if.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/vtoc.h \
	$(INC)/sys/open.h \
	$(INC)/sys/file.h \
	$(INC)/sys/tuneable.h \
	$(INC)/sys/uio.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/ddi.h \
	$(INC)/sys/inline.h \
	$(FRC)

iuart.o: iuart.c \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/iu.h \
	$(INC)/sys/csr.h \
	$(INC)/sys/dma.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/file.h \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/termio.h \
	$(INC)/sys/conf.h \
	$(INC)/sys/firmware.h \
	$(INC)/sys/nvram.h \
	$(INC)/sys/inline.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/strtty.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/eucioctl.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/ddi.h \
	$(FRC)

log.o: log.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/strstat.h \
	$(INC)/sys/log.h \
	$(INC)/sys/strlog.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/inline.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/file.h \
	$(INC)/sys/ddi.h \
	$(INC)/sys/syslog.h \
	$(FRC)

mau.o: mau.c \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/reg.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/siginfo.h \
	$(INC)/sys/user.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/mau.h \
	$(FRC)

mem.o: mem.c \
	$(INC)/sys/types.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/param.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/ddi.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/user.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/firmware.h \
	$(INC)/sys/iobd.h \
	$(INC)/sys/uio.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/disp.h \
	$(INC)/sys/mman.h \
	$(INC)/vm/hat.h \
	$(INC)/vm/as.h \
	$(INC)/vm/seg.h \
	$(INC)/vm/seg_vn.h \
	$(INC)/vm/seg_kmem.h \
	$(FRC)

prf.o: prf.c \
	$(INC)/sys/signal.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/nvram.h \
	$(INC)/sys/user.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/uio.h \
	$(INC)/sys/cred.h \
	$(FRC)

sad.o:	sad.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/file.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/user.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/conf.h \
	$(INC)/sys/sad.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/ddi.h \
	$(FRC)

sddrv.o: sddrv.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/csr.h \
	$(INC)/vm/page.h \
	$(INC)/sys/uio.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/ddi.h \
	$(FRC)

sockmod.o: sockmod.c \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/socketvar.h \
	$(INC)/sys/tihdr.h \
	$(INC)/sys/timod.h \
	$(INC)/sys/sockmod.h \
	$(INC)/sys/socket.h \
	$(INC)/sys/tiuser.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/user.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/sysmacros.h \
	$(FRC)
	$(CC) $(CFLAGS) -DAF_UNIX -c sockmod.c

sp.o: sp.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/stream.h \
	$(FRC)

stubs.o: stubs.c \
	$(FRC)

sxt.o: sxt.c \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/conf.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/dir.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/user.h \
	$(INC)/sys/termio.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/strtty.h \
	$(INC)/sys/sxt.h \
	$(INC)/sys/cmn_err.h \
	$(FRC)

timod.o: timod.c \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/tihdr.h \
	$(INC)/sys/timod.h \
	$(INC)/sys/tiuser.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/user.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/systm.h \
	$(FRC)

tirdwr.o: tirdwr.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/tihdr.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/user.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/errno.h \
	$(FRC)
