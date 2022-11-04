#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)netinet:netinet/ip.mk	1.7"
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
INC = /usr/include
INSDIR = /boot
MASTERD = ../master.d
MKBOOT = mkboot
PFLAGS = -DSYSV -Du3b2 -D_KERNEL $(MORECPP) -I$(INC)
DASHO = -O
CFLAGS = $(DASHO) $(PFLAGS)
CC = cc
LD = ld
FRC = 

PRODUCTS = IP ICMP RAWIP
OBJ = in.o in_cksum.o in_pcb.o in_switch.o in_transp.o ip_input.o \
	ip_output.o ip_main.o netlib.o route.o
ROBJ=	raw_ip_main.o raw_ip.o raw_ip_cb.o
CLEAN = ip.o ip_icmp.o icmp.o $(OBJ) $(ROBJ)

.c.o:
	$(CC) $(CFLAGS) -c $*.c

all : $(PRODUCTS)

IP:	ip.o $(MASTERD)/ip
	$(MKBOOT) -m $(MASTERD) -d . ip.o

RAWIP:	rawip.o $(MASTERD)/rawip
	$(MKBOOT) -m $(MASTERD) -d . rawip.o

ICMP:	ip_icmp.o $(MASTERD)/icmp
	rm -f icmp.o
	ln ip_icmp.o icmp.o
	$(MKBOOT) -m $(MASTERD) -d . icmp.o
	rm icmp.o

ip.o: $(OBJ)
	$(LD) -r $(UTSLDFLAGS) -o ip.o $(OBJ)

rawip.o:	$(ROBJ)
	$(LD) -r $(UTSLDFLAGS) -o rawip.o $(ROBJ)

ip_icmp.o:	ip_icmp.c \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/time.h \
	$(INC)/sys/log.h \
	$(INC)/net/if.h \
	$(INC)/net/if_arp.h \
	$(INC)/net/route.h \
	$(INC)/net/strioc.h \
	$(INC)/netinet/icmp_var.h \
	$(INC)/netinet/in.h \
	$(INC)/netinet/in_pcb.h \
	$(INC)/netinet/in_systm.h \
	$(INC)/netinet/in_var.h \
	$(INC)/netinet/ip.h \
	$(INC)/netinet/ip_icmp.h \
	$(INC)/netinet/ip_str.h \
	$(INC)/sys/param.h \
	$(INC)/sys/socket.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/strlog.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/tihdr.h \
	$(INC)/sys/tiuser.h \
	$(INC)/sys/types.h

ip_main.o:	ip_main.c \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/fs/s5dir.h \
	$(INC)/sys/lihdr.h \
	$(INC)/sys/log.h \
	$(INC)/net/if.h \
	$(INC)/net/if_arp.h \
	$(INC)/net/route.h \
	$(INC)/net/strioc.h \
	$(INC)/netinet/in.h \
	$(INC)/netinet/in_var.h \
	$(INC)/netinet/ip_str.h \
	$(INC)/netinet/ip_var.h \
	$(INC)/sys/param.h \
	$(INC)/sys/time.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/socket.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/strlog.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/types.h \
	$(INC)/sys/user.h

ip_input.o:	ip_input.c \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/lihdr.h \
	$(INC)/sys/log.h \
	$(INC)/net/if.h \
	$(INC)/net/if_arp.h \
	$(INC)/net/route.h \
	$(INC)/netinet/in.h \
	$(INC)/netinet/in_pcb.h \
	$(INC)/netinet/in_systm.h \
	$(INC)/netinet/in_var.h \
	$(INC)/netinet/insrem.h \
	$(INC)/netinet/ip.h \
	$(INC)/netinet/ip_icmp.h \
	$(INC)/netinet/ip_str.h \
	$(INC)/netinet/ip_var.h \
	$(INC)/netinet/tcp.h \
	$(INC)/sys/param.h \
	$(INC)/sys/socket.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/strlog.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/types.h

in.o:		in.c \
	$(INC)/sys/errno.h \
	$(INC)/net/af.h \
	$(INC)/net/if.h \
	$(INC)/net/if_arp.h \
	$(INC)/net/route.h \
	$(INC)/netinet/in.h \
	$(INC)/netinet/in_systm.h \
	$(INC)/netinet/in_var.h \
	$(INC)/netinet/ip_str.h \
	$(INC)/sys/param.h \
	$(INC)/sys/socket.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/types.h

route.o:	route.c \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/ioctl.h \
	$(INC)/sys/log.h \
	$(INC)/net/af.h \
	$(INC)/net/if.h \
	$(INC)/net/if_arp.h \
	$(INC)/net/route.h \
	$(INC)/netinet/in.h \
	$(INC)/netinet/in_var.h \
	$(INC)/netinet/ip_str.h \
	$(INC)/sys/param.h \
	$(INC)/sys/socket.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/strlog.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/types.h

in_cksum.o:	in_cksum.c \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/types.h

in_pcb.o:	in_pcb.c \
	$(INC)/sys/errno.h \
	$(INC)/sys/fs/s5dir.h \
	$(INC)/sys/log.h \
	$(INC)/net/if.h \
	$(INC)/net/if_arp.h \
	$(INC)/net/route.h \
	$(INC)/netinet/in.h \
	$(INC)/netinet/in_pcb.h \
	$(INC)/netinet/in_systm.h \
	$(INC)/netinet/in_var.h \
	$(INC)/netinet/insrem.h \
	$(INC)/netinet/ip.h \
	$(INC)/netinet/ip_str.h \
	$(INC)/netinet/ip_var.h \
	$(INC)/sys/param.h \
	$(INC)/sys/socket.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/strlog.h \
	$(INC)/sys/tihdr.h \
	$(INC)/sys/tiuser.h \
	$(INC)/sys/types.h

in_switch.o:	in_switch.c \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/fs/s5dir.h \
	$(INC)/sys/inline.h \
	$(INC)/sys/ioctl.h \
	$(INC)/sys/lihdr.h \
	$(INC)/sys/log.h \
	$(INC)/net/af.h \
	$(INC)/net/if.h \
	$(INC)/net/if_arp.h \
	$(INC)/net/route.h \
	$(INC)/netinet/in.h \
	$(INC)/netinet/in_var.h \
	$(INC)/netinet/ip_str.h \
	$(INC)/sys/param.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/socket.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/strlog.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/types.h \
	$(INC)/sys/user.h

in_transp.o:	in_transp.c \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/fs/s5dir.h \
	$(INC)/sys/lihdr.h \
	$(INC)/sys/log.h \
	$(INC)/net/if.h \
	$(INC)/net/if_arp.h \
	$(INC)/net/route.h \
	$(INC)/net/strioc.h \
	$(INC)/netinet/in.h \
	$(INC)/netinet/in_var.h \
	$(INC)/netinet/ip_str.h \
	$(INC)/netinet/ip_var.h \
	$(INC)/sys/param.h \
	$(INC)/sys/time.h \
	$(INC)/sys/timod.h \
	$(INC)/sys/pcb.h \
	$(INC)/sys/psw.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/socket.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/strlog.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/types.h \
	$(INC)/sys/user.h

#insrem.o:	insrem.c \
	#$(INC)/netinet/insrem.h \
	#$(INC)/sys/socket.h \
	#$(INC)/sys/types.h

raw_ip_cb.o:	raw_ip_cb.c \
	$(INC)/sys/types.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/socket.h \
	$(INC)/sys/stream.h \
	$(INC)/netinet/in.h \
	$(INC)/netinet/in_pcb.h \
	$(INC)/net/route.h

raw_ip.o:	raw_ip.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/strlog.h \
	$(INC)/sys/log.h \
	$(INC)/sys/tihdr.h \
	$(INC)/sys/tiuser.h \
	$(INC)/sys/socket.h \
	$(INC)/netinet/in.h \
	$(INC)/net/route.h \
	$(INC)/net/if.h \
	$(INC)/netinet/in_pcb.h \
	$(INC)/netinet/ip_var.h \
	$(INC)/netinet/in_var.h \
	$(INC)/netinet/ip_str.h \
	$(INC)/netinet/ip.h \
	$(INC)/netinet/in_systm.h

raw_ip_main.o:	raw_ip_main.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/strlog.h \
	$(INC)/sys/log.h \
	$(INC)/sys/tihdr.h \
	$(INC)/sys/tiuser.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/socket.h \
	$(INC)/netinet/in.h \
	$(INC)/net/route.h \
	$(INC)/net/if.h \
	$(INC)/net/strioc.h \
	$(INC)/netinet/in_pcb.h \
	$(INC)/netinet/ip_var.h \
	$(INC)/netinet/in_var.h \
	$(INC)/netinet/ip_str.h \
	$(INC)/netinet/ip.h \
	$(INC)/netinet/in_systm.h


clean:
		rm -f $(CLEAN)

clobber:	clean
		rm -f $(PRODUCTS)

