#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kernel:os/os.mk	1.59.1.9"

ROOT = 
STRIP = strip
INC = $(ROOT)/usr/include
MKBOOT = mkboot
MASTERD = ../master.d
LIBNAME = ../lib.os
DASHO = -O
CFLAGS = -I$(INC) -D_KERNEL $(MORECPP) $(DASHO)
PFLAGS = -I$(INC) -D_KERNEL $(MORECPP)
FRC =

FILES = \
	$(LIBNAME)(acct.o) \
	$(LIBNAME)(bio.o) \
	$(LIBNAME)(bitmap.o) \
	$(LIBNAME)(bitmasks.o) \
	$(LIBNAME)(clock.o) \
	$(LIBNAME)(cmn_err.o) \
	$(LIBNAME)(core.o) \
	$(LIBNAME)(cred.o) \
	$(LIBNAME)(cxenix.o) \
	$(LIBNAME)(exec.o) \
	$(LIBNAME)(exit.o) \
	$(LIBNAME)(fbio.o) \
	$(LIBNAME)(fio.o) \
	$(LIBNAME)(flock.o) \
	$(LIBNAME)(fork.o) \
	$(LIBNAME)(getsizes.o) \
	$(LIBNAME)(grow.o) \
	$(LIBNAME)(kperf.o) \
	$(LIBNAME)(list.o) \
	$(LIBNAME)(local.o) \
	$(LIBNAME)(lock.o) \
	$(LIBNAME)(machdep.o) \
	$(LIBNAME)(main.o) \
	$(LIBNAME)(malloc.o) \
	$(LIBNAME)(move.o) \
	$(LIBNAME)(pgrp.o) \
	$(LIBNAME)(pipe.o) \
	$(LIBNAME)(procset.o) \
	$(LIBNAME)(scalls.o) \
	$(LIBNAME)(sched.o) \
	$(LIBNAME)(session.o) \
	$(LIBNAME)(sig.o) \
	$(LIBNAME)(slp.o) \
	$(LIBNAME)(space.o) \
	$(LIBNAME)(startup.o) \
	$(LIBNAME)(streamio.o) \
	$(LIBNAME)(strsubr.o) \
	$(LIBNAME)(subr.o) \
	$(LIBNAME)(sys3b.o) \
	$(LIBNAME)(sysent.o) \
	$(LIBNAME)(todc.o) \
	$(LIBNAME)(trap.o) \
	$(LIBNAME)(vm_meter.o) \
	$(LIBNAME)(vm_pageout.o) \
	$(LIBNAME)(vm_subr.o) \
	$(LIBNAME)(xsys.o)

#
# Files for avoiding optimization
#

NOPFILES = \
	cmn_err


all:	DDI IPC KMA MSG SEM SHM pick $(LIBNAME) name.o

.PRECIOUS:	$(LIBNAME)

DDI:	../ddi.o

IPC:	../ipc.o $(MASTERD)/ipc
	$(MKBOOT) -m $(MASTERD) -d . ../ipc.o; 

KMA:	../kma.o $(MASTERD)/kma
	$(MKBOOT) -m $(MASTERD) -d . ../kma.o

MSG:	../msg.o $(MASTERD)/msg
	$(MKBOOT) -m $(MASTERD) -d . ../msg.o

SEM:	../sem.o $(MASTERD)/sem
	$(MKBOOT) -m $(MASTERD) -d . ../sem.o;

SHM:	../shm.o $(MASTERD)/shm
	$(MKBOOT) -m $(MASTERD) -d . ../shm.o;

# included for development, should be removed for distribution

pick:
	-@if [ "`ls *.o 2>/dev/null`" ] ; \
	then \
		echo "\tar ruv $(LIBNAME) *.o" ; \
		$(AR) ruv $(LIBNAME) *.o ; \
		echo "\trm -f *.o" ; \
		rm -f *.o ; \
	fi

$(LIBNAME):	$(FILES)
	$(AR) ruv $(LIBNAME) *.o ;
	rm -f *.o ;

name.o:
	$(CC) $(CFLAGS) -c name.c \
		-DREL=`expr '"$(REL)' : '\(..\{0,8\}\)'`\" \
		-DVER=`expr '"$(VER)' : '\(..\{0,8\}\)'`\"
	mv name.o ..

.c.a:
	@case $* in \
	$(NOPFILES))\
		echo "\t$(CC) $(PFLAGS) -c $<";\
		$(CC) $(PFLAGS) -c $<;\
		exit $$?;\
		;;\
	getsizes) \
		echo "\t$(CC) $(PFLAGS) -c -g $<"; \
		$(CC) $(PFLAGS) -c -g $<; \
		if [ $$? -ne 0 ]; \
		then \
			exit $$?; \
		fi; \
		echo "\tcp $*.o ../$*.o"; \
		cp $*.o ../$*.o; \
		exit $$?; \
		;; \
	*)\
		echo "\t$(CC) $(CFLAGS) -c $<";\
		$(CC) $(CFLAGS) -c $<;\
		exit $$?;\
		;;\
	esac

clean:
	-rm -f *.o

clobber:	clean
	-rm -f $(LIBNAME) ../ddi.o ../name.o ../getsizes.o
	-rm -f ../ipc.o ../kma.o ../msg.o ../sem.o ../shm.o 
	-rm -f IPC MSG SEM SHM 

FRC:
	-rm -f $(LIBNAME)
	$(AR) qc $(LIBNAME)

#
# Header dependencies
#

../ddi.o: ddi.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/sysinfo.h \
	$(INC)/sys/map.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/user.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/uio.h \
	$(INC)/sys/kmem.h \
	$(INC)/sys/conf.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/file.h \
	$(INC)/sys/ddi.h \
	$(INC)/sys/mkdev.h \
	$(FRC)
	$(CC) $(CFLAGS) -c ddi.c
	mv ddi.o ../ddi.o	# save this, but not here

../ipc.o: ipc.c \
	$(INC)/sys/errno.h \
	$(INC)/sys/types.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/param.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/user.h \
	$(INC)/sys/ipc.h \
	$(FRC)
	$(CC) $(CFLAGS) -c ipc.c
	mv ipc.o ../ipc.o	# Save, but not here

../kma.o: kma.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/kmem.h \
	$(INC)/sys/tuneable.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/sysinfo.h \
	$(INC)/sys/inline.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/strsubr.h \
	$(INC)/sys/systm.h \
	$(FRC)
	$(CC) $(CFLAGS) -c kma.c
	mv kma.o ../kma.o	# Save, but not here

../msg.o: msg.c \
	$(INC)/sys/types.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/param.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/user.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/time.h \
	$(INC)/sys/map.h \
	$(INC)/sys/ipc.h \
	$(INC)/sys/msg.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/sysinfo.h \
	$(INC)/sys/disp.h \
	$(FRC)
	$(CC) $(CFLAGS) -c msg.c
	mv msg.o ../msg.o	# Save, but not here

../sem.o: sem.c \
	$(INC)/sys/types.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/param.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/map.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/time.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/ipc.h \
	$(INC)/sys/sem.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/user.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/sysinfo.h \
	$(FRC)
	$(CC) $(CFLAGS) -c sem.c
	mv sem.o ../sem.o	# Save, but not here

../shm.o: shm.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/time.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/kmem.h \
	$(INC)/sys/user.h \
	$(INC)/sys/ipc.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/shm.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/tuneable.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/vm.h \
	$(INC)/sys/mman.h \
	$(INC)/vm/hat.h \
	$(INC)/vm/seg.h \
	$(INC)/vm/as.h \
	$(INC)/vm/seg_vn.h \
	$(INC)/vm/anon.h \
	$(INC)/vm/page.h \
	$(INC)/vm/vpage.h \
	$(FRC)
	$(CC) $(CFLAGS) -c shm.c
	mv shm.o ../shm.o	# Save, but not here

$(LIBNAME)(acct.o): acct.c \
	$(INC)/sys/types.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/param.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/acct.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/user.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/fstyp.h \
	$(INC)/sys/file.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/conf.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/resource.h \
	$(INC)/sys/uio.h \
	$(FRC)

$(LIBNAME)(bio.o): bio.c \
	$(INC)/sys/types.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/param.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/disp.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/sysinfo.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/user.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/iobuf.h \
	$(INC)/sys/conf.h \
	$(INC)/sys/var.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/inline.h \
	$(INC)/sys/kmem.h \
	$(INC)/vm/page.h \
	$(FRC)

$(LIBNAME)(bitmap.o): bitmap.c \
	$(INC)/sys/types.h \
	$(INC)/sys/bitmap.h \
	$(FRC)

$(LIBNAME)(bitmasks.o): bitmasks.c \
	$(FRC)

$(LIBNAME)(clock.o): clock.c \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/tuneable.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/sysinfo.h \
	$(INC)/sys/callo.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/user.h \
	$(INC)/sys/time.h \
	$(INC)/sys/evecb.h \
	$(INC)/sys/hrtcntl.h \
	$(INC)/sys/conf.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/var.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/map.h \
	$(INC)/sys/swap.h \
	$(INC)/sys/inline.h \
	$(INC)/sys/disp.h \
	$(INC)/sys/class.h \
	$(INC)/sys/fs/rf_acct.h \
	$(INC)/sys/time.h \
	$(INC)/sys/debug.h \
	$(INC)/vm/anon.h \
	$(FRC)

$(LIBNAME)(cmn_err.o): cmn_err.c \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/time.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/conf.h \
	$(INC)/sys/panregs.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/sit.h \
	$(INC)/sys/csr.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/edt.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/nvram.h \
	$(INC)/sys/firmware.h \
	$(INC)/sys/tty.h \
	$(INC)/sys/reg.h \
	$(INC)/sys/inline.h \
	$(FRC)

$(LIBNAME)(core.o): core.c \
	$(INC)/sys/exec.h \
	$(INC)/sys/rf_messg.h \
	$(INC)/sys/siginfo.h \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/user.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/fault.h \
	$(INC)/sys/syscall.h \
	$(INC)/sys/ucontext.h \
	$(INC)/sys/procfs.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/reg.h \
	$(INC)/sys/var.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/inline.h \
	$(INC)/sys/uio.h \
	$(INC)/sys/exec.h \
	$(INC)/sys/mman.h \
	$(INC)/vm/as.h \
	$(FRC)

$(LIBNAME)(cred.o): cred.c \
	$(INC)/sys/inline.h \
	$(INC)/sys/types.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/param.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/file.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/kmem.h \
	$(INC)/sys/user.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/var.h \
	$(INC)/sys/conf.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/acct.h \
	$(INC)/sys/fault.h \
	$(INC)/sys/syscall.h \
	$(INC)/sys/procfs.h \
	$(INC)/sys/dl.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/tuneable.h \
	$(FRC)

$(LIBNAME)(cxenix.o): cxenix.c \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/user.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/reg.h \
	$(FRC)

$(LIBNAME)(exec.o): exec.c \
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
	$(INC)/sys/rf_messg.h \
	$(INC)/sys/conf.h \
	$(INC)/sys/uio.h \
	$(INC)/sys/pathname.h \
	$(INC)/sys/disp.h \
	$(INC)/sys/vm.h \
	$(INC)/sys/mman.h \
	$(INC)/vm/hat.h \
	$(INC)/vm/anon.h \
	$(INC)/vm/as.h \
	$(INC)/vm/seg.h \
	$(INC)/vm/page.h \
	$(INC)/vm/seg_vn.h \
	$(FRC)

$(LIBNAME)(fbio.o): fbio.c \
	$(INC)/sys/types.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/conf.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/fbuf.h \
	$(INC)/sys/param.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/vnode.h \
	$(INC)/vm/seg.h \
	$(INC)/vm/seg_kmem.h \
	$(INC)/vm/seg_map.h \
	$(FRC)

$(LIBNAME)(exit.o): exit.c \
	$(INC)/sys/siginfo.h \
	$(INC)/sys/session.h \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/user.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/fault.h \
	$(INC)/sys/syscall.h \
	$(INC)/sys/ucontext.h \
	$(INC)/sys/procfs.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/acct.h \
	$(INC)/sys/var.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/inline.h \
	$(INC)/sys/wait.h \
	$(INC)/sys/procset.h \
	$(INC)/sys/disp.h \
	$(INC)/sys/class.h \
	$(INC)/sys/events.h \
	$(INC)/sys/evsys.h \
	$(INC)/sys/hrtsys.h \
	$(INC)/vm/page.h \
	$(FRC)

$(LIBNAME)(fio.o): fio.c \
	$(INC)/sys/types.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/param.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/user.h \
	$(INC)/sys/file.h \
	$(INC)/sys/conf.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/var.h \
	$(INC)/sys/sysinfo.h \
	$(INC)/sys/acct.h \
	$(INC)/sys/open.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/evecb.h \
	$(INC)/sys/hrtcntl.h \
	$(INC)/sys/priocntl.h \
	$(INC)/sys/procset.h \
	$(INC)/sys/events.h \
	$(INC)/sys/evsys.h \
	$(INC)/sys/asyncsys.h \
	$(FRC)

$(LIBNAME)(flock.o): flock.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/file.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/fstyp.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/user.h \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/flock.h \
	$(INC)/sys/conf.h \
	$(INC)/sys/systm.h \
	$(FRC)

$(LIBNAME)(fork.o): fork.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/user.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/sysinfo.h \
	$(INC)/sys/map.h \
	$(INC)/sys/file.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/var.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/time.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/acct.h \
	$(INC)/sys/tuneable.h \
	$(INC)/sys/inline.h \
	$(INC)/sys/disp.h \
	$(INC)/sys/class.h \
	$(INC)/sys/kmem.h \
	$(INC)/sys/session.h \
	$(INC)/sys/fault.h \
	$(INC)/sys/syscall.h \
	$(INC)/sys/ucontext.h \
	$(INC)/sys/procfs.h \
	$(INC)/vm/hat.h \
	$(INC)/vm/as.h \
	$(INC)/vm/seg_u.h \
	$(FRC)

$(LIBNAME)(getsizes.o): getsizes.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/tty.h \
	$(INC)/sys/file.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/map.h \
	$(INC)/sys/callo.h \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/flock.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/user.h \
	$(INC)/sys/var.h \
	$(INC)/sys/tuneable.h \
	$(INC)/sys/conf.h \
	$(INC)/sys/ipc.h \
	$(INC)/sys/msg.h \
	$(INC)/sys/sem.h \
	$(INC)/sys/shm.h \
	$(INC)/sys/swap.h \
	$(INC)/sys/iobuf.h \
	$(INC)/sys/nserve.h \
	$(INC)/sys/dirent.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/strsubr.h \
	$(INC)/sys/evecb.h \
	$(INC)/sys/hrtcntl.h \
	$(INC)/sys/hrtsys.h \
	$(INC)/sys/priocntl.h \
	$(INC)/sys/procset.h \
	$(INC)/sys/events.h \
	$(INC)/sys/evsyscall.h \
	$(INC)/sys/evsys.h \
	$(INC)/sys/disp.h \
	$(INC)/sys/class.h \
	$(INC)/vm/as.h \
	$(INC)/vm/seg.h \
	$(INC)/vm/page.h \
	$(INC)/vm/seg_vn.h \
	$(FRC)

$(LIBNAME)(grow.o): grow.c \
	$(INC)/sys/types.h \
	$(INC)/sys/bitmasks.h \
	$(INC)/sys/param.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/user.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/var.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/tuneable.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/vm.h \
	$(INC)/sys/file.h \
	$(INC)/sys/mman.h \
	$(INC)/vm/hat.h \
	$(INC)/vm/as.h \
	$(INC)/vm/page.h \
	$(INC)/vm/seg.h \
	$(INC)/vm/seg_dev.h \
	$(INC)/vm/seg_vn.h \
	$(FRC)

$(LIBNAME)(kperf.o): kperf.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/kmem.h \
	$(INC)/sys/tuneable.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/inline.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/reg.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/user.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/mount.h \
	$(INC)/sys/fstyp.h \
	$(INC)/sys/var.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/utsname.h \
	$(INC)/sys/sysinfo.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/map.h \
	$(INC)/sys/swap.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/conf.h \
	$(INC)/sys/inline.h \
	$(FRC)

$(LIBNAME)(list.o): list.c \
	$(INC)/sys/list.h \
	$(FRC)

$(LIBNAME)(local.o): local.c \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/user.h \
	$(INC)/sys/errno.h \
	$(FRC)

$(LIBNAME)(lock.o): lock.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/user.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/lock.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/tuneable.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/vm/hat.h \
	$(INC)/vm/as.h \
	$(INC)/vm/seg.h \
	$(INC)/vm/page.h \
	$(FRC)

$(LIBNAME)(machdep.o): machdep.c \
	$(INC)/sys/siginfo.h \
	$(INC)/sys/ucontext.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/csr.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/sit.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/time.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/map.h \
	$(INC)/sys/reg.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/dma.h \
	$(INC)/sys/utsname.h \
	$(INC)/sys/acct.h \
	$(INC)/sys/file.h \
	$(INC)/sys/fstyp.h \
	$(INC)/sys/user.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/edt.h \
	$(INC)/sys/firmware.h \
	$(INC)/sys/var.h \
	$(INC)/sys/inline.h \
	$(INC)/sys/conf.h \
	$(FRC)

$(LIBNAME)(main.o): main.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/user.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/file.h \
	$(INC)/sys/evecb.h \
	$(INC)/sys/hrtcntl.h \
	$(INC)/sys/hrtsys.h \
	$(INC)/sys/priocntl.h \
	$(INC)/sys/procset.h \
	$(INC)/sys/events.h \
	$(INC)/sys/evsys.h \
	$(INC)/sys/asyncsys.h \
	$(INC)/sys/var.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/conf.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/vm/as.h \
	$(INC)/vm/seg_vn.h \
	$(FRC)

$(LIBNAME)(malloc.o): malloc.c \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/map.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/debug.h \
	$(FRC)

$(LIBNAME)(move.o): move.c \
	$(INC)/sys/types.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/param.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/time.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/user.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/conf.h \
	$(INC)/sys/file.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/uio.h \
	$(FRC)

$(LIBNAME)(name.o): name.c \
	$(FRC)

$(LIBNAME)(pgrp.o): pgrp.c \
	$(INC)/sys/siginfo.h \
	$(INC)/sys/session.h \
	$(INC)/sys/types.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/param.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/file.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/fstyp.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/siginfo.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/user.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/mount.h \
	$(INC)/sys/var.h \
	$(INC)/sys/conf.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/fault.h \
	$(INC)/sys/syscall.h \
	$(INC)/sys/ucontext.h \
	$(INC)/sys/procfs.h \
	$(INC)/sys/session.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/strsubr.h \
	$(FRC)

$(LIBNAME)(pipe.o): pipe.c \
	$(INC)/sys/types.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/param.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/user.h \
	$(INC)/sys/fstyp.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/file.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/conf.h \
	$(INC)/sys/fs/fifonode.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/strsubr.h \
	$(FRC)

$(LIBNAME)(procset.o): procset.c \
	$(INC)/sys/types.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/param.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/file.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/siginfo.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/kmem.h \
	$(INC)/sys/user.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/var.h \
	$(INC)/sys/conf.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/acct.h \
	$(INC)/sys/procset.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/wait.h \
	$(INC)/sys/fault.h \
	$(INC)/sys/syscall.h \
	$(INC)/sys/procfs.h \
	$(INC)/sys/ucontext.h \
	$(INC)/sys/session.h \
	$(FRC)

$(LIBNAME)(scalls.o): scalls.c \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/tuneable.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/user.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/time.h \
	$(INC)/sys/file.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/var.h \
	$(INC)/sys/clock.h \
	$(INC)/sys/inline.h \
	$(INC)/sys/conf.h \
	$(INC)/sys/uio.h \
	$(INC)/sys/uadmin.h \
	$(INC)/sys/utsname.h \
	$(INC)/sys/utssys.h \
	$(INC)/sys/ustat.h \
	$(INC)/sys/statvfs.h \
	$(INC)/sys/fault.h \
	$(INC)/sys/syscall.h \
	$(INC)/sys/ucontext.h \
	$(INC)/sys/procfs.h \
	$(INC)/sys/procset.h \
	$(INC)/sys/siginfo.h \
	$(INC)/sys/session.h \
	$(INC)/sys/class.h \
	$(INC)/sys/evecb.h \
	$(INC)/sys/hrtcntl.h \
	$(INC)/sys/disp.h \
	$(INC)/sys/systeminfo.h \
	$(INC)/vm/hat.h \
	$(INC)/vm/as.h \
	$(INC)/vm/seg.h \
	$(FRC)

$(LIBNAME)(sched.o): sched.c \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/sysinfo.h \
	$(INC)/sys/var.h \
	$(INC)/sys/tuneable.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/inline.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/disp.h \
	$(INC)/sys/class.h \
	$(FRC)

$(LIBNAME)(session.o): session.c \
	$(INC)/sys/types.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/param.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/evecb.h \
	$(INC)/sys/hrtcntl.h \
	$(INC)/sys/priocntl.h \
	$(INC)/sys/events.h \
	$(INC)/sys/evsys.h \
	$(INC)/sys/file.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/user.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/var.h \
	$(INC)/sys/conf.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/strsubr.h \
	$(INC)/sys/session.h \
	$(INC)/sys/priocntl.h \
	$(INC)/sys/kmem.h \
	$(FRC)

$(LIBNAME)(sig.o): sig.c \
	$(INC)/sys/kmem.h \
	$(INC)/sys/siginfo.h \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/user.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/fault.h \
	$(INC)/sys/syscall.h \
	$(INC)/sys/ucontext.h \
	$(INC)/sys/procfs.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/reg.h \
	$(INC)/sys/var.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/rf_messg.h \
	$(INC)/sys/inline.h \
	$(INC)/sys/uio.h \
	$(INC)/sys/wait.h \
	$(INC)/sys/class.h \
	$(INC)/sys/disp.h \
	$(INC)/sys/mman.h \
	$(INC)/sys/procset.h \
	$(INC)/sys/kmem.h \
	$(INC)/sys/evecb.h \
	$(INC)/sys/hrtcntl.h \
	$(INC)/sys/events.h \
	$(INC)/sys/evsys.h \
	$(INC)/vm/as.h \
	$(FRC)

$(LIBNAME)(slp.o): slp.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/user.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/sysinfo.h \
	$(INC)/sys/map.h \
	$(INC)/sys/file.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/inline.h \
	$(INC)/sys/disp.h \
	$(INC)/sys/class.h \
	$(INC)/sys/fault.h \
	$(INC)/sys/syscall.h \
	$(INC)/sys/ucontext.h \
	$(INC)/sys/procfs.h \
	$(INC)/sys/priocntl.h \
	$(INC)/sys/evecb.h \
	$(INC)/sys/hrtcntl.h \
	$(INC)/sys/events.h \
	$(INC)/sys/evsys.h \
	$(FRC)

$(LIBNAME)(space.o): space.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/acct.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/sysinfo.h \
	$(INC)/sys/swap.h \
	$(INC)/sys/fs/rf_acct.h \
	$(FRC)

$(LIBNAME)(startup.o): startup.c \
	$(INC)/sys/session.h \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/boot.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/firmware.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/nvram.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/user.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/map.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/reg.h \
	$(INC)/sys/utsname.h \
	$(INC)/sys/tty.h \
	$(INC)/sys/var.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/disp.h \
	$(INC)/sys/class.h \
	$(INC)/sys/mman.h \
	$(INC)/sys/file.h \
	$(INC)/sys/uio.h \
	$(INC)/vm/seg.h \
	$(INC)/vm/seg_kmem.h \
	$(INC)/vm/seg_vn.h \
	$(INC)/vm/seg_u.h \
	$(INC)/vm/seg_map.h \
	$(INC)/vm/page.h \
	$(FRC)

$(LIBNAME)(streamio.o): streamio.c \
	$(INC)/sys/session.h \
	$(INC)/sys/types.h \
	$(INC)/sys/file.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/param.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/user.h \
	$(INC)/sys/conf.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/strsubr.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/strstat.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/var.h \
	$(INC)/sys/poll.h \
	$(INC)/sys/termio.h \
	$(INC)/sys/ttold.h \
	$(INC)/sys/inline.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/uio.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/sad.h \
	$(INC)/sys/priocntl.h \
	$(INC)/sys/hrtcntl.h \
	$(INC)/sys/procset.h \
	$(INC)/sys/events.h \
	$(INC)/sys/evsys.h \
	$(INC)/sys/siginfo.h \
	$(FRC)

$(LIBNAME)(strsubr.o): strsubr.c \
	$(INC)/sys/session.h \
	$(INC)/sys/types.h \
	$(INC)/sys/file.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/param.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/user.h \
	$(INC)/sys/conf.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/strsubr.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/strstat.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/var.h \
	$(INC)/sys/poll.h \
	$(INC)/sys/termio.h \
	$(INC)/sys/ttold.h \
	$(INC)/sys/inline.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/uio.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/sad.h \
	$(INC)/sys/priocntl.h \
	$(INC)/sys/hrtcntl.h \
	$(INC)/sys/procset.h \
	$(INC)/sys/events.h \
	$(INC)/sys/evsys.h \
	$(INC)/sys/siginfo.h \
	$(FRC)

$(LIBNAME)(subr.o): subr.c \
	$(INC)/sys/types.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/param.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/file.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/kmem.h \
	$(INC)/sys/user.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/var.h \
	$(INC)/sys/conf.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/acct.h \
	$(INC)/sys/fault.h \
	$(INC)/sys/syscall.h \
	$(INC)/sys/procfs.h \
	$(INC)/sys/dl.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/tuneable.h \
	$(FRC)

$(LIBNAME)(sys3b.o): sys3b.c \
	$(INC)/sys/mman.h \
	$(INC)/vm/as.h \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/iu.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/user.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/time.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/edt.h \
	$(INC)/sys/firmware.h \
	$(INC)/sys/sys3b.h \
	$(INC)/sys/nvram.h \
	$(INC)/sys/utsname.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/boothdr.h \
	$(INC)/sys/uadmin.h \
	$(INC)/sys/map.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/swap.h \
	$(INC)/sys/gate.h \
	$(INC)/sys/var.h \
	$(INC)/sys/tuneable.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/inline.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/conf.h \
	$(INC)/sys/fstyp.h \
	$(INC)/sys/uio.h \
	$(INC)/vm/seg.h \
	$(FRC)

$(LIBNAME)(sysent.o): sysent.c \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/systm.h \
	$(FRC)

$(LIBNAME)(todc.o): todc.c \
	$(INC)/sys/sbd.h \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/time.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/firmware.h \
	$(INC)/sys/sys3b.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/nvram.h \
	$(INC)/sys/todc.h \
	$(INC)/sys/cmn_err.h \
	$(FRC)

$(LIBNAME)(trap.o): trap.c \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/csr.h \
	$(INC)/sys/sit.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/user.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/siginfo.h \
	$(INC)/sys/fault.h \
	$(INC)/sys/syscall.h \
	$(INC)/sys/ucontext.h \
	$(INC)/sys/prsystm.h \
	$(INC)/sys/reg.h \
	$(INC)/sys/sysinfo.h \
	$(INC)/sys/edt.h \
	$(INC)/sys/utsname.h \
	$(INC)/sys/firmware.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/var.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/inline.h \
	$(INC)/sys/mau.h \
	$(INC)/sys/disp.h \
	$(INC)/sys/class.h \
	$(INC)/sys/kmem.h \
	$(INC)/sys/vmsystm.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/evecb.h \
	$(INC)/sys/hrtcntl.h \
	$(INC)/sys/priocntl.h \
	$(INC)/sys/events.h \
	$(INC)/sys/evsys.h \
	$(INC)/sys/sys3b.h \
	$(INC)/vm/as.h \
	$(INC)/vm/seg.h \
	$(INC)/sys/immu.h \
	$(INC)/vm/seg_vn.h \
	$(INC)/vm/seg_kmem.h \
	$(INC)/vm/faultcode.h \
	$(FRC)

$(LIBNAME)(vm_meter.o): vm_meter.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/time.h \
	$(INC)/sys/proc.h \
	$(INC)/vm/kernel.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/vm.h \
	$(INC)/vm/hat.h \
	$(INC)/vm/as.h \
	$(INC)/vm/rm.h \
	$(INC)/sys/var.h \
	$(FRC)

$(LIBNAME)(vm_pageout.o): vm_pageout.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/uio.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/mman.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/vm.h \
	$(INC)/sys/vmparam.h \
	$(INC)/vm/trace.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/immu.h \
	$(INC)/vm/hat.h \
	$(INC)/vm/as.h \
	$(INC)/vm/seg.h \
	$(INC)/vm/page.h \
	$(INC)/vm/pvn.h \
	$(FRC)

$(LIBNAME)(vm_subr.o): vm_subr.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/file.h \
	$(INC)/sys/uio.h \
	$(INC)/sys/user.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/sysinfo.h \
	$(INC)/sys/mman.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/vm.h \
	$(INC)/vm/hat.h \
	$(INC)/vm/as.h \
	$(INC)/vm/seg.h \
	$(INC)/vm/page.h \
	$(INC)/vm/seg_vn.h \
	$(INC)/vm/seg_kmem.h \
	$(INC)/vm/seg_u.h \
	$(FRC)

$(LIBNAME)(xsys.o): xsys.c \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/user.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/locking.h \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/timeb.h \
	$(INC)/sys/flock.h \
	$(INC)/sys/conf.h \
	$(INC)/sys/fstyp.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/file.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/proctl.h \
	$(INC)/sys/var.h \
	$(FRC)
