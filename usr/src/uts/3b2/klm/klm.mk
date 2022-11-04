#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)klm:klm/klm.mk	1.2"
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
ROOT =
STRIP = strip
INC = $(ROOT)/usr/include
MKBOOT = mkboot
MASTERD = ../master.d

DASHG =
DASHO = -O
PFLAGS = $(DASHG) -D_KERNEL -DSYSV $(MORECPP) -DLOCKDEBUG
CFLAGS = $(DASHO) $(PFLAGS)
DEFLIST =
FRC =

FILES = \
	klm_kprot.o \
	klm_lkmgr.o

all:	KLM

KLM:	klm.o $(MASTERD)/klm
	$(MKBOOT) -m $(MASTERD) -d . klm.o

klm.o:	$(FILES)
	$(LD) -r -o klm.o $(FILES)

.c.o:
	$(CC) $(DEFLIST) -I$(INC) $(CFLAGS) -c $*.c

clean:
	-rm -f *.o

clobber:	clean
	-rm -f KLM

#
# Header dependencies
#

klm_kprot.o: klm_kprot.c \
	$(INC)/rpc/types.h \
	$(INC)/sys/types.h \
	$(INC)/sys/kmem.h \
	$(INC)/sys/time.h \
	$(INC)/sys/tiuser.h \
	$(INC)/sys/fcntl.h \
	$(INC)/netinet/in.h \
	$(INC)/sys/t_kuser.h \
	$(INC)/rpc/xdr.h \
	$(INC)/rpc/auth.h \
	$(INC)/rpc/clnt.h \
	$(INC)/rpc/rpc_msg.h \
	$(INC)/rpc/auth_sys.h \
	$(INC)/rpc/auth_des.h \
	$(INC)/rpc/svc.h \
	$(INC)/rpc/svc_auth.h \
	$(INC)/rpcsvc/klm_prot.h \
	$(FRC)

klm_lkmgr.o: klm_lkmgr.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/user.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/socket.h \
	$(INC)/sys/socketvar.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/file.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/rpc/pmap_prot.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/rpc/types.h \
	$(INC)/netinet/in.h \
	$(INC)/rpc/xdr.h \
	$(INC)/rpc/auth.h \
	$(INC)/rpc/clnt.h \
	$(INC)/klm/lockmgr.h \
	$(INC)/rpcsvc/klm_prot.h \
	$(INC)/net/if.h \
	$(INC)/nfs/nfs.h \
	$(INC)/nfs/nfs_clnt.h \
	$(INC)/nfs/rnode.h \
	$(FRC)
