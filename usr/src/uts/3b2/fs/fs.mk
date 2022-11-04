#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)fs:fs/fs.mk	1.33"
ROOT =
STRIP = strip
INC = $(ROOT)/usr/include
MKBOOT = mkboot
MASTERD = ../master.d
CC = cc

DASHG =
DASHO = -O
PFLAGS = $(DASHG) -D_KERNEL $(MORECPP)
CFLAGS = $(DASHO) $(PFLAGS)
DEFLIST =
FRC =

FILES = \
	dnlc.o \
	fs_subr.o \
	fsflush.o \
	lookup.o \
	pathname.o \
	strcalls.o \
	vfs.o \
	vncalls.o \
	vnode.o

all:	fs.o fstypes

fs.o: $(FILES)
	$(LD) -r -o fs.o $(FILES)

.c.o:
	$(CC) $(DEFLIST) -I$(INC) $(CFLAGS) -c $*.c

fstypes:
	@for i in `ls`;\
	do\
		if [ -d $$i -a -f $$i/$$i.mk ];then\
		case $$i in\
		*.*)\
			;;\
		*)\
		cd  $$i;\
			echo "====== $(MAKE) -f $$i.mk \"MAKE=$(MAKE)\" \"AS=$(AS)\" \"CC=$(CC)\" \"LD=$(LD)\" \"FRC=$(FRC)\" \"INC=$(INC)\" \"MORECPP=$(MORECPP)\" \"DASHO=$(DASHO)\"";\
		$(MAKE) -f $$i.mk "MAKE=$(MAKE)" "AS=$(AS)" "CC=$(CC)" "LD=$(LD)" "FRC=$(FRC)" "INC=$(INC)" "MORECPP=$(MORECPP)" "DASHO=$(DASHO)"; \
		cd .. ;; \
		esac;\
		fi;\
	done

clean:
	-rm -f *.o
	@for i in `ls`; \
	do \
		if [ -d $$i -a -f $$i/$$i.mk ]; then \
			cd $$i; \
			$(MAKE) -f $$i.mk clean; \
			cd ..; \
		fi; \
	done

clobber:	clean
	@for i in `ls`; \
	do \
		if [ -d $$i -a -f $$i/$$i.mk ]; then \
			cd $$i; \
			$(MAKE) -f $$i.mk clobber; \
			cd ..; \
		fi; \
	done

#
# Header dependencies
#

dnlc.o: dnlc.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/dnlc.h \
	$(INC)/sys/kmem.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/user.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/cmn_err.h \
	$(FRC)

fs_subr.o: fs_subr.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/conf.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/flock.h \
	$(INC)/sys/uio.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/file.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/user.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/poll.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/list.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/rf_comm.h \
	$(INC)/fs/fs_subr.h \
	$(FRC)

fsflush.o: fsflush.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/tuneable.h \
	$(INC)/sys/inline.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/user.h \
	$(INC)/sys/var.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/kmem.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/swap.h \
	$(INC)/sys/vm.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/sysinfo.h \
	$(INC)/vm/hat.h \
	$(INC)/vm/page.h \
	$(INC)/vm/pvn.h \
	$(FRC)

lookup.o: lookup.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/sysinfo.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/user.h \
	$(INC)/sys/uio.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/pathname.h \
	$(INC)/sys/conf.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/disp.h \
	$(FRC)

pathname.o: pathname.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/uio.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/pathname.h \
	$(INC)/sys/kmem.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/vnode.h \
	$(FRC)

strcalls.o: strcalls.c \
	$(INC)/sys/types.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/param.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/user.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/file.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/var.h \
	$(INC)/sys/conf.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/stream.h \
	$(FRC)

vfs.o: vfs.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/user.h \
	$(INC)/sys/uio.h \
	$(INC)/sys/fstyp.h \
	$(INC)/sys/kmem.h \
	$(INC)/sys/inline.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/pathname.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/mount.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/statvfs.h \
	$(INC)/sys/statfs.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/dnlc.h \
	$(INC)/sys/file.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/debug.h \
	$(FRC)

vncalls.o: vncalls.c \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/user.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/time.h \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/pathname.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/ttold.h \
	$(INC)/sys/var.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/file.h \
	$(INC)/sys/mode.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/sysinfo.h \
	$(INC)/sys/inline.h \
	$(INC)/sys/uio.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/poll.h \
	$(INC)/sys/kmem.h \
	$(INC)/sys/filio.h \
	$(INC)/sys/locking.h \
	$(INC)/sys/disp.h \
	$(INC)/sys/mkdev.h \
	$(INC)/sys/time.h \
	$(INC)/rpc/types.h \
	$(INC)/nfs/nfs.h \
	$(FRC)

vnode.o: vnode.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/user.h \
	$(INC)/sys/uio.h \
	$(INC)/sys/file.h \
	$(INC)/sys/pathname.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/mode.h \
	$(INC)/sys/conf.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/disp.h \
	$(INC)/sys/systm.h \
	$(FRC)
