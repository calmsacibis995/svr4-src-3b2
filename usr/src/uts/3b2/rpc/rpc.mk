#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)krpc:krpc/krpc.mk	1.8"
#
#	@(#)rpc.mk 1.4 89/01/03 SMI
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
#	Kernel RPC
#
ROOT =
STRIP = strip
MKBOOT = mkboot
MASTERD = ../master.d
BOOTD = .
INC = $(ROOT)/usr/include
PFLAGS = -I$(INC) -D_KERNEL $(MORECPP)
CFLAGS = $(DASHO) $(PFLAGS) -DSYSV
DEFLIST =
FRC =

KRPCOBJ = clnt_clts.o clnt_gen.o svc_gen.o svc_clts.o \
	  xdr_mblk.o xdr_mem.o svc.o  auth_kern.o rpc_prot.o \
	  rpc_calmsg.o xdr.o svc_auth.o authu_prot.o \
	  svcauthdes.o svc_authu.o xdr_array.o key_call.o \
	  key_prot.o clnt_perr.o \
	  auth_des.o authdesprt.o authdesubr.o rpc_subr.o

all:	KRPC

KRPC:	krpc.o $(MASTERD)/krpc
	$(MKBOOT) -m $(MASTERD) -d $(BOOTD) krpc.o

lint:
	lint $(CFLAGS) -Dlint *.c

krpc.o:	$(KRPCOBJ)
	$(LD) -r -o krpc.o $(KRPCOBJ)

clean:
	-rm -f *.o

clobber:	clean
	-rm -f KRPC

#
# Header dependencies -- make depend should build these!
#

auth_des.o: auth_des.c \
	$(INC)/sys/types.h \
	$(INC)/sys/time.h \
	$(INC)/sys/socket.h \
	$(INC)/rpc/des_crypt.h \
	$(INC)/rpc/types.h \
	$(INC)/rpc/auth.h \
	$(INC)/rpc/auth_des.h \
	$(INC)/rpc/xdr.h \
	$(INC)/netinet/in.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/debug.h \
	$(FRC)

auth_kern.o: auth_kern.c \
	$(INC)/sys/param.h \
	$(INC)/sys/time.h \
	$(INC)/sys/types.h \
	$(INC)/rpc/types.h \
	$(INC)/sys/user.h \
	$(INC)/sys/proc.h \
	$(INC)/rpc/xdr.h \
	$(INC)/rpc/auth.h \
	$(INC)/rpc/auth_unix.h \
	$(INC)/netinet/in.h \
	$(INC)/sys/utsname.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/kmem.h \
	$(INC)/sys/sysmacros.h \
	$(FRC)

authdesprt.o: authdesprt.c \
	$(INC)/rpc/types.h \
	$(INC)/rpc/xdr.h \
	$(INC)/rpc/auth.h \
	$(INC)/rpc/auth_des.h \
	$(FRC)

authdesubr.o: authdesubr.c \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/time.h \
	$(INC)/sys/socketvar.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/user.h \
	$(INC)/sys/socket.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/netinet/in.h \
	$(INC)/rpc/rpc.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/strsubr.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/utsname.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/uio.h \
	$(INC)/sys/systeminfo.h \
	$(FRC)

authu_prot.o: authu_prot.c \
	$(INC)/rpc/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/time.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/user.h \
	$(INC)/rpc/xdr.h \
	$(INC)/rpc/auth.h \
	$(INC)/rpc/auth_unix.h \
	$(INC)/sys/utsname.h \
	$(FRC)

clnt_clts.o: clnt_clts.c \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/user.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/socket.h \
	$(INC)/sys/socketvar.h \
	$(INC)/rpc/types.h \
	$(INC)/rpc/xdr.h \
	$(INC)/rpc/auth.h \
	$(INC)/rpc/clnt.h \
	$(INC)/sys/file.h \
	$(INC)/rpc/rpc_msg.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/strsubr.h \
	$(INC)/sys/tiuser.h \
	$(INC)/sys/tihdr.h \
	$(INC)/sys/t_kuser.h \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/kmem.h \
	$(FRC)

clnt_gen.o: clnt_gen.c \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/rpc/types.h \
	$(INC)/netinet/in.h \
	$(INC)/rpc/auth.h \
	$(INC)/rpc/clnt.h \
	$(INC)/sys/tiuser.h \
	$(INC)/sys/t_kuser.h \
	$(INC)/rpc/svc.h \
	$(INC)/rpc/xdr.h \
	$(INC)/sys/file.h \
	$(INC)/sys/user.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/tihdr.h \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/socket.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/cred.h \
	$(FRC)

clnt_perr.o: clnt_perr.c \
	$(INC)/sys/types.h \
	$(INC)/rpc/types.h \
	$(INC)/rpc/auth.h \
	$(INC)/rpc/clnt.h \
	$(FRC)

key_call.o: key_call.c \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/socket.h \
	$(INC)/sys/time.h \
	$(INC)/rpc/rpc.h \
	$(INC)/rpc/key_prot.h \
	$(INC)/sys/user.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/uio.h \
	$(INC)/sys/debug.h \
	$(FRC)

key_prot.o: key_prot.c \
	$(INC)/sys/types.h \
	$(INC)/rpc/rpc.h \
	$(INC)/rpc/key_prot.h \
	$(FRC)

rpc_calmsg.o: rpc_calmsg.c \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/rpc/types.h \
	$(INC)/rpc/xdr.h \
	$(INC)/rpc/auth.h \
	$(INC)/rpc/clnt.h \
	$(INC)/rpc/rpc_msg.h \
	$(INC)/netinet/in.h \
	$(FRC)

rpc_prot.o: rpc_prot.c \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/rpc/types.h \
	$(INC)/rpc/xdr.h \
	$(INC)/rpc/auth.h \
	$(INC)/rpc/clnt.h \
	$(INC)/rpc/rpc_msg.h \
	$(INC)/netinet/in.h \
	$(FRC)

svc.o: svc.c \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/rpc/types.h \
	$(INC)/sys/socket.h \
	$(INC)/sys/socketvar.h \
	$(INC)/sys/siginfo.h \
	$(INC)/sys/time.h \
	$(INC)/netinet/in.h \
	$(INC)/rpc/xdr.h \
	$(INC)/rpc/auth.h \
	$(INC)/rpc/clnt.h \
	$(INC)/rpc/rpc_msg.h \
	$(INC)/sys/tiuser.h \
	$(INC)/sys/t_kuser.h \
	$(INC)/rpc/svc.h \
	$(INC)/rpc/svc_auth.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/user.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/strsubr.h \
	$(INC)/sys/tihdr.h \
	$(INC)/sys/debug.h \
	$(FRC)

svc_auth.o: svc_auth.c \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/rpc/types.h \
	$(INC)/netinet/in.h \
	$(INC)/rpc/xdr.h \
	$(INC)/rpc/auth.h \
	$(INC)/rpc/clnt.h \
	$(INC)/rpc/rpc_msg.h \
	$(INC)/sys/tiuser.h \
	$(INC)/sys/tihdr.h \
	$(INC)/sys/t_kuser.h \
	$(INC)/rpc/svc.h \
	$(INC)/rpc/svc_auth.h \
	$(FRC)

svc_authu.o: svc_authu.c \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/time.h \
	$(INC)/netinet/in.h \
	$(INC)/rpc/types.h \
	$(INC)/rpc/xdr.h \
	$(INC)/rpc/auth.h \
	$(INC)/rpc/clnt.h \
	$(INC)/rpc/rpc_msg.h \
	$(INC)/sys/tiuser.h \
	$(INC)/sys/tihdr.h \
	$(INC)/sys/t_kuser.h \
	$(INC)/rpc/svc.h \
	$(INC)/rpc/auth_unix.h \
	$(INC)/rpc/svc_auth.h \
	$(FRC)

svc_clts.o: svc_clts.c \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/rpc/types.h \
	$(INC)/netinet/in.h \
	$(INC)/rpc/xdr.h \
	$(INC)/rpc/auth.h \
	$(INC)/rpc/clnt.h \
	$(INC)/rpc/rpc_msg.h \
	$(INC)/sys/tiuser.h \
	$(INC)/sys/t_kuser.h \
	$(INC)/rpc/svc.h \
	$(INC)/sys/socket.h \
	$(INC)/sys/socketvar.h \
	$(INC)/sys/file.h \
	$(INC)/sys/user.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/tihdr.h \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/kmem.h \
	$(FRC)

svc_gen.o: svc_gen.c \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/rpc/types.h \
	$(INC)/netinet/in.h \
	$(INC)/rpc/auth.h \
	$(INC)/rpc/clnt.h \
	$(INC)/sys/tiuser.h \
	$(INC)/sys/t_kuser.h \
	$(INC)/rpc/svc.h \
	$(INC)/sys/file.h \
	$(INC)/sys/user.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/tihdr.h \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/errno.h \
	$(FRC)

svcauthdes.o: svcauthdes.c \
	$(INC)/rpc/des_crypt.h \
	$(INC)/sys/types.h \
	$(INC)/sys/time.h \
	$(INC)/sys/param.h \
	$(INC)/netinet/in.h \
	$(INC)/rpc/types.h \
	$(INC)/rpc/xdr.h \
	$(INC)/rpc/auth.h \
	$(INC)/rpc/auth_des.h \
	$(INC)/rpc/svc_auth.h \
	$(INC)/sys/tiuser.h \
	$(INC)/sys/tihdr.h \
	$(INC)/sys/t_kuser.h \
	$(INC)/rpc/svc.h \
	$(INC)/rpc/rpc_msg.h \
	$(FRC)

xdr.o: xdr.c \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/rpc/types.h \
	$(INC)/rpc/xdr.h \
	$(FRC)

xdr_array.o: xdr_array.c \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/rpc/types.h \
	$(INC)/rpc/xdr.h \
	$(FRC)

xdr_mblk.o: xdr_mblk.c \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/rpc/types.h \
	$(INC)/sys/stream.h \
	$(INC)/rpc/xdr.h \
	$(INC)/netinet/in.h \
	$(FRC)

xdr_mem.o: xdr_mem.c \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/netinet/in.h \
	$(INC)/rpc/types.h \
	$(INC)/rpc/xdr.h \
	$(FRC)

rpc_subr.o: rpc_subr.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(FRC)
