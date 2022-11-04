#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kernel:vm/vm.mk	1.14"

ROOT =
INC = $(ROOT)/usr/include
MKBOOT = mkboot
MASTERD = ../master.d
LIBNAME = ../lib.vm
DASHO = -O
CFLAGS = -I$(INC) -D_KERNEL $(MORECPP) $(DASHO)
PFLAGS = -I$(INC) -D_KERNEL $(MORECPP)
FRC =

FILES = \
	$(LIBNAME)(seg_dev.o) \
	$(LIBNAME)(seg_kmem.o) \
	$(LIBNAME)(seg_map.o) \
	$(LIBNAME)(seg_u.o) \
	$(LIBNAME)(seg_vn.o) \
	$(LIBNAME)(vm_anon.o) \
	$(LIBNAME)(vm_as.o) \
	$(LIBNAME)(vm_hat.o) \
	$(LIBNAME)(vm_machdep.o) \
	$(LIBNAME)(vm_mp.o) \
	$(LIBNAME)(vm_page.o) \
	$(LIBNAME)(vm_pvn.o) \
	$(LIBNAME)(vm_rm.o) \
	$(LIBNAME)(vm_seg.o) \
	$(LIBNAME)(vm_swap.o) \
	$(LIBNAME)(vm_vpage.o)


all:		pick $(LIBNAME)

.PRECIOUS:	$(LIBNAME)

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

.c.a:
		$(CC) -c $(CFLAGS) $<

clean:
	-rm -f *.o

clobber: clean
	-rm -f $(LIBNAME)

FRC:
	-rm -f $(LIBNAME)
	$(AR) qc $(LIBNAME)


$(LIBNAME)(seg_dev.o): seg_dev.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/mman.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/kmem.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/user.h \
	$(INC)/vm/hat.h \
	$(INC)/vm/as.h \
	$(INC)/vm/seg.h \
	$(INC)/vm/seg_dev.h \
	$(INC)/vm/pvn.h \
	$(INC)/vm/vpage.h \
	$(FRC)

$(LIBNAME)(seg_kmem.o): seg_kmem.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/vm.h \
	$(INC)/sys/user.h \
	$(INC)/sys/mman.h \
	$(INC)/sys/map.h \
	$(INC)/sys/tuneable.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/immu.h \
	$(INC)/vm/seg_kmem.h \
	$(INC)/vm/hat.h \
	$(INC)/vm/as.h \
	$(INC)/vm/seg.h \
	$(INC)/vm/anon.h \
	$(INC)/vm/rm.h \
	$(INC)/vm/page.h \
	$(FRC)

$(LIBNAME)(seg_map.o): seg_map.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/mman.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/kmem.h \
	$(INC)/vm/trace.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/debug.h \
	$(INC)/vm/seg_kmem.h \
	$(INC)/vm/hat.h \
	$(INC)/vm/as.h \
	$(INC)/vm/seg.h \
	$(INC)/vm/seg_map.h \
	$(INC)/vm/page.h \
	$(INC)/vm/pvn.h \
	$(INC)/vm/rm.h \
	$(FRC)

$(LIBNAME)(seg_u.o): seg_u.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/mman.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/tuneable.h \
	$(INC)/vm/anon.h \
	$(INC)/vm/rm.h \
	$(INC)/vm/page.h \
	$(INC)/vm/seg.h \
	$(INC)/vm/as.h \
	$(INC)/sys/swap.h \
	$(INC)/sys/immu.h \
	$(INC)/vm/vm_hat.h \
	$(INC)/vm/seg_u.h \
	$(INC)/sys/proc.h \
	$(FRC)

$(LIBNAME)(seg_vn.o): seg_vn.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/mman.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/vmsystm.h \
	$(INC)/sys/swap.h \
	$(INC)/sys/kmem.h \
	$(INC)/sys/inline.h \
	$(INC)/sys/tuneable.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/vm/trace.h \
	$(INC)/vm/hat.h \
	$(INC)/vm/mp.h \
	$(INC)/vm/as.h \
	$(INC)/vm/seg.h \
	$(INC)/vm/seg_vn.h \
	$(INC)/vm/pvn.h \
	$(INC)/vm/anon.h \
	$(INC)/vm/page.h \
	$(INC)/vm/vpage.h \
	$(INC)/vm/seg_kmem.h \
	$(FRC)

$(LIBNAME)(vm_anon.o): vm_anon.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/mman.h \
	$(INC)/sys/time.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/vmmeter.h \
	$(INC)/sys/swap.h \
	$(INC)/sys/tuneable.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/disp.h \
	$(INC)/vm/hat.h \
	$(INC)/vm/anon.h \
	$(INC)/vm/as.h \
	$(INC)/vm/page.h \
	$(INC)/vm/seg.h \
	$(INC)/vm/pvn.h \
	$(INC)/vm/rm.h \
	$(INC)/vm/mp.h \
	$(INC)/vm/trace.h \
	$(INC)/vm/vmlog.h \
	$(FRC)

$(LIBNAME)(vm_as.o): vm_as.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/immu.h \
	$(INC)/vm/hat.h \
	$(INC)/vm/as.h \
	$(INC)/vm/seg.h \
	$(INC)/vm/seg_vn.h \
	$(FRC)

$(LIBNAME)(vm_machdep.o): vm_machdep.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/user.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/vm.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/debug.h \
	$(INC)/vm/hat.h \
	$(INC)/vm/as.h \
	$(INC)/vm/seg.h \
	$(INC)/vm/page.h \
	$(INC)/vm/seg_vn.h \
	$(INC)/vm/seg_kmem.h \
	$(INC)/sys/immu.h \
	$(INC)/vm/cpu.h \
	$(FRC)

$(LIBNAME)(vm_hat.o): vm_hat.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/immu.h \
	$(INC)/vm/vm_hat.h \
	$(INC)/vm/hat.h \
	$(INC)/vm/seg.h \
	$(INC)/vm/as.h \
	$(INC)/vm/page.h \
	$(INC)/sys/mman.h \
	$(INC)/sys/bitmasks.h \
	$(INC)/sys/tuneable.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/inline.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/fs/s5dir.h \
	$(INC)/sys/user.h \
	$(INC)/sys/systm.h \
	$(FRC)

$(LIBNAME)(vm_page.o): vm_page.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/time.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/vm.h \
	$(INC)/vm/trace.h \
	$(INC)/sys/swap.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/tuneable.h \
	$(INC)/sys/disp.h \
	$(INC)/vm/hat.h \
	$(INC)/vm/anon.h \
	$(INC)/vm/page.h \
	$(INC)/vm/seg.h \
	$(INC)/vm/pvn.h \
	$(INC)/vm/mp.h \
	$(INC)/vm/vmlog.h \
	$(INC)/vm/reboot.h \
	$(INC)/sys/systm.h \
	$(INC)/vm/debugger.h \
	$(FRC)

$(LIBNAME)(vm_pvn.o): vm_pvn.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/time.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/uio.h \
	$(INC)/sys/vmmeter.h \
	$(INC)/sys/vmsystm.h \
	$(INC)/sys/mman.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/kmem.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/debug.h \
	$(INC)/vm/trace.h \
	$(INC)/vm/hat.h \
	$(INC)/vm/as.h \
	$(INC)/vm/seg.h \
	$(INC)/vm/rm.h \
	$(INC)/vm/pvn.h \
	$(INC)/vm/page.h \
	$(INC)/vm/seg_map.h \
	$(INC)/vm/vmlog.h \
	$(INC)/vm/kernel.h \
	$(INC)/sys/systm.h \
	$(FRC)

$(LIBNAME)(vm_rm.o): vm_rm.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/user.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/vm/hat.h \
	$(INC)/vm/as.h \
	$(INC)/vm/rm.h \
	$(INC)/vm/seg.h \
	$(INC)/vm/page.h \
	$(FRC)

$(LIBNAME)(vm_seg.o): vm_seg.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/immu.h \
	$(INC)/vm/hat.h \
	$(INC)/vm/as.h \
	$(INC)/vm/seg.h \
	$(INC)/vm/mp.h \
	$(FRC)

$(LIBNAME)(vm_swap.o): vm_swap.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/sbd.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/user.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/kmem.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/file.h \
	$(INC)/sys/uio.h \
	$(INC)/sys/conf.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/tuneable.h \
	$(INC)/vm/bootconf.h \
	$(INC)/vm/trace.h \
	$(INC)/vm/as.h \
	$(INC)/vm/seg.h \
	$(INC)/vm/page.h \
	$(INC)/vm/seg_vn.h \
	$(INC)/vm/hat.h \
	$(INC)/vm/anon.h \
	$(INC)/sys/swap.h \
	$(INC)/vm/seg_map.h \
	$(FRC)

$(LIBNAME)(vm_vpage.o): vm_vpage.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/vm/vpage.h \
	$(INC)/vm/mp.h \
	$(FRC)

