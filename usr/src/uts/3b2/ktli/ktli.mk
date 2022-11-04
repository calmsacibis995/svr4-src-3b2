#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ktli:ktli/ktli.mk	1.2"
#
#	@(#)ktli.mk 1.2 89/01/11 SMI
#
#
#  		PROPRIETARY NOTICE (Combined)
#  
#  This source code is unpublished proprietary information
#  constituting, or derived under license from AT&T's Unix(r) System V.
#  In addition, portions of such source code were derived from Berkeley
#  4.3 BSD under license from the Regents of the University of
#  California.
#  
#  
#  
#  		Copyright Notice 
#  
#  Notice of copyright on this source code product does not indicate 
#  publication.
#  
#  	(c) 1986,1987,1988,1989  Sun Microsystems, Inc.
#  	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
#  	          All rights reserved.
#
#
#	Kernel TLI inteface
#
STRIP = strip
MKBOOT = mkboot
MASTERD = ../master.d
BOOTD = .
PFLAGS = -I$(INC) -D_KERNEL $(MORECPP)
CFLAGS = $(DASHO) $(PFLAGS) -DSYSV
DEFLIST =
FRC =

KTLIOBJ = t_kclose.o t_kgtstate.o t_ksndudat.o t_kutil.o t_kalloc.o \
	  t_kconnect.o t_kopen.o t_kspoll.o t_kbind.o t_kfree.o \
	  t_krcvudat.o 

all:	KTLI

KTLI:	ktli.o
	$(MKBOOT) -m $(MASTERD) -d $(BOOTD) ktli.o

lint:
	lint $(CFLAGS) -Dlint *.c

ktli.o:	$(KTLIOBJ)
	$(LD) -r -o ktli.o $(KTLIOBJ)

clean:
	rm -f *.o

clobber:	clean
	rm -f KTLI


#
# Header dependencies -- make depend should build these!
#

t_kalloc.o: t_kalloc.c \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/user.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/ioctl.h \
	$(INC)/sys/file.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/tihdr.h \
	$(INC)/sys/timod.h \
	$(INC)/sys/tiuser.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/t_kuser.h \
	$(INC)/sys/kmem.h

t_kbind.o: t_kbind.c \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/file.h \
	$(INC)/sys/user.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/strsubr.h \
	$(INC)/sys/ioctl.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/tihdr.h \
	$(INC)/sys/timod.h \
	$(INC)/sys/tiuser.h \
	$(INC)/sys/t_kuser.h \
	$(INC)/sys/kmem.h

t_kclose.o: t_kclose.c \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/user.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/ioctl.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/file.h \
	$(INC)/sys/tihdr.h \
	$(INC)/sys/timod.h \
	$(INC)/sys/tiuser.h \
	$(INC)/sys/t_kuser.h

t_kconnect.o: t_kconnect.c \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/user.h \
	$(INC)/sys/file.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/ioctl.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/tihdr.h \
	$(INC)/sys/timod.h \
	$(INC)/sys/tiuser.h \
	$(INC)/sys/t_kuser.h

t_kfree.o: t_kfree.c \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/user.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/ioctl.h \
	$(INC)/sys/file.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/tihdr.h \
	$(INC)/sys/timod.h \
	$(INC)/sys/tiuser.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/t_kuser.h

t_kgtstate.o: t_kgtstate.c \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/file.h \
	$(INC)/sys/user.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/ioctl.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/strsubr.h \
	$(INC)/sys/tihdr.h \
	$(INC)/sys/timod.h \
	$(INC)/sys/tiuser.h \
	$(INC)/sys/t_kuser.h

t_kopen.o: t_kopen.c \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/file.h \
	$(INC)/sys/user.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/ioctl.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/strsubr.h \
	$(INC)/sys/tihdr.h \
	$(INC)/sys/timod.h \
	$(INC)/sys/tiuser.h \
	$(INC)/sys/t_kuser.h \
	$(INC)/sys/kmem.h

t_krcvudat.o: t_krcvudat.c \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/user.h \
	$(INC)/sys/file.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/ioctl.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/tihdr.h \
	$(INC)/sys/timod.h \
	$(INC)/sys/tiuser.h \
	$(INC)/sys/t_kuser.h

t_ksndudat.o: t_ksndudat.c \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/user.h \
	$(INC)/sys/file.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/strsubr.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/ioctl.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/tihdr.h \
	$(INC)/sys/timod.h \
	$(INC)/sys/tiuser.h \
	$(INC)/sys/t_kuser.h

t_kspoll.o: t_kspoll.c \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/file.h \
	$(INC)/sys/user.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/ioctl.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/strsubr.h \
	$(INC)/sys/tihdr.h \
	$(INC)/sys/timod.h \
	$(INC)/sys/tiuser.h \
	$(INC)/sys/t_kuser.h

t_kutil.o: t_kutil.c \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/file.h \
	$(INC)/sys/user.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/ioctl.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/strsubr.h \
	$(INC)/sys/tihdr.h \
	$(INC)/sys/timod.h \
	$(INC)/sys/tiuser.h \
	$(INC)/sys/t_kuser.h
