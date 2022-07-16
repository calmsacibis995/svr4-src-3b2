#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)exec:exec/elf/elf.mk	1.3"
ROOT =
STRIP = strip
INC = $(ROOT)/usr/include
MKBOOT = mkboot
MASTERD = ../../master.d

DASHG =
DASHO = -O
PFLAGS = $(DASHG) -D_KERNEL $(MORECPP)
CFLAGS = $(DASHO) -I$(INC) $(PFLAGS)
FRC =


all:	ELF

ELF:	elf.o $(MASTERD)/elf
	$(MKBOOT) -m $(MASTERD) -d . elf.o

clean:
	-rm -f *.o

clobber:	clean
	-rm -f ELF

#
# Header dependencies
#


elf.o: elf.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/map.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/user.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/file.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/fstyp.h \
	$(INC)/sys/acct.h \
	$(INC)/sys/sysinfo.h \
	$(INC)/sys/reg.h \
	$(INC)/sys/var.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/tuneable.h \
	$(INC)/sys/tty.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/conf.h \
	$(INC)/sys/uio.h \
	$(INC)/sys/pathname.h \
	$(INC)/sys/exec.h \
	$(FRC)

