#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)netinet:netinet/tcp.mk	1.3"
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

PRODUCTS = TCP
OBJ = tcp_debug.o tcp_input.o tcp_main.o tcp_output.o tcp_state.o \
	tcp_subr.o tcp_timer.o

CLEAN = tcp.o $(OBJ)

.c.o:
	$(CC) $(CFLAGS) -c $*.c

all : $(PRODUCTS)

TCP: tcp.o $(MASTERD)/tcp
	$(MKBOOT) -m $(MASTERD) -d . tcp.o

tcp.o: $(OBJ)
	$(LD) -r $(UTSLDFLAGS) -o tcp.o $(OBJ)


tcp_debug.o:	tcp_debug.c $(INC)/sys/errno.h \
    $(INC)/sys/log.h $(INC)/net/route.h \
     \
	$(INC)/netinet/in_pcb.h $(INC)/netinet/in_systm.h  \
	$(INC)/netinet/ip.h $(INC)/netinet/ip_var.h \
	$(INC)/netinet/tcp.h $(INC)/netinet/tcp_debug.h \
	$(INC)/netinet/tcp_fsm.h $(INC)/netinet/tcp_seq.h \
	$(INC)/netinet/tcp_timer.h $(INC)/netinet/tcp_var.h \
	$(INC)/netinet/tcpip.h $(INC)/sys/param.h \
	$(INC)/sys/socket.h $(INC)/sys/stream.h \
	$(INC)/sys/strlog.h $(INC)/sys/stropts.h \
	$(INC)/sys/types.h $(INC)/netinet/in.h 


tcp_main.o:	tcp_main.c $(INC)/sys/conf.h $(INC)/sys/debug.h \
	$(INC)/sys/errno.h $(INC)/sys/fs/s5dir.h $(INC)/sys/inline.h \
	$(INC)/sys/log.h $(INC)/sys/map.h $(INC)/net/if.h \
	$(INC)/net/if_arp.h $(INC)/net/route.h \
	$(INC)/net/strioc.h \
	$(INC)/netinet/in.h $(INC)/netinet/in_pcb.h \
	$(INC)/netinet/in_systm.h $(INC)/netinet/in_var.h \
	$(INC)/netinet/ip_str.h $(INC)/netinet/ip_var.h \
	$(INC)/netinet/tcp.h $(INC)/netinet/tcp_debug.h \
	$(INC)/netinet/tcp_fsm.h $(INC)/netinet/tcp_seq.h \
	$(INC)/netinet/tcp_timer.h $(INC)/netinet/tcp_var.h \
	$(INC)/netinet/tcpip.h \
	$(INC)/sys/param.h $(INC)/sys/pcb.h \
	$(INC)/sys/psw.h $(INC)/sys/signal.h \
	$(INC)/sys/socket.h $(INC)/sys/stream.h $(INC)/sys/strlog.h \
	$(INC)/sys/stropts.h $(INC)/sys/sysmacros.h $(INC)/sys/systm.h \
	$(INC)/sys/tihdr.h $(INC)/sys/types.h $(INC)/sys/user.h

tcp_state.o:	tcp_state.c $(INC)/sys/cmn_err.h $(INC)/sys/debug.h \
	$(INC)/sys/errno.h $(INC)/sys/inline.h $(INC)/sys/log.h \
	$(INC)/net/if.h $(INC)/net/if_arp.h \
	$(INC)/net/route.h \
	$(INC)/netinet/in.h \
	$(INC)/netinet/in_pcb.h $(INC)/netinet/in_systm.h \
	$(INC)/netinet/in_var.h $(INC)/netinet/ip.h \
	$(INC)/netinet/ip_str.h $(INC)/netinet/ip_var.h \
	$(INC)/netinet/tcp.h $(INC)/netinet/tcp_debug.h \
	$(INC)/netinet/tcp_fsm.h $(INC)/netinet/tcp_seq.h \
	$(INC)/netinet/tcp_timer.h $(INC)/netinet/tcp_var.h \
	$(INC)/netinet/tcpip.h \
	$(INC)/sys/param.h $(INC)/sys/socket.h \
	$(INC)/sys/stream.h $(INC)/sys/strlog.h $(INC)/sys/tihdr.h \
	$(INC)/sys/tiuser.h $(INC)/sys/types.h

tcp_input.o:	tcp_input.c $(INC)/sys/cmn_err.h $(INC)/sys/errno.h \
	$(INC)/sys/inline.h $(INC)/sys/log.h $(INC)/net/if.h \
	$(INC)/net/if_arp.h  \
	$(INC)/net/route.h  \
	$(INC)/netinet/in.h $(INC)/netinet/in_pcb.h \
	$(INC)/netinet/in_systm.h $(INC)/netinet/in_var.h \
	$(INC)/netinet/insrem.h $(INC)/netinet/ip.h \
	$(INC)/netinet/ip_str.h $(INC)/netinet/ip_var.h \
	$(INC)/netinet/tcp.h $(INC)/netinet/tcp_debug.h \
	$(INC)/netinet/tcp_fsm.h $(INC)/netinet/tcp_seq.h \
	$(INC)/netinet/tcp_timer.h $(INC)/netinet/tcp_var.h \
	$(INC)/netinet/tcpip.h $(INC)/sys/param.h $(INC)/sys/socket.h \
	$(INC)/sys/stream.h $(INC)/sys/strlog.h $(INC)/sys/stropts.h \
	$(INC)/sys/tihdr.h $(INC)/sys/types.h

tcp_output.o:	tcp_output.c $(INC)/sys/cmn_err.h $(INC)/sys/debug.h \
	$(INC)/sys/errno.h $(INC)/sys/inline.h $(INC)/sys/log.h \
	$(INC)/net/if.h $(INC)/net/if_arp.h \
	$(INC)/net/route.h \
	$(INC)/netinet/in.h $(INC)/netinet/in_pcb.h \
	$(INC)/netinet/in_systm.h $(INC)/netinet/in_var.h \
	$(INC)/netinet/ip.h $(INC)/netinet/ip_str.h \
	$(INC)/netinet/ip_var.h $(INC)/netinet/tcp.h \
	$(INC)/netinet/tcp_debug.h $(INC)/netinet/tcp_fsm.h \
	$(INC)/netinet/tcp_seq.h $(INC)/netinet/tcp_timer.h \
	$(INC)/netinet/tcp_var.h $(INC)/netinet/tcpip.h \
	$(INC)/sys/param.h \
	$(INC)/sys/socket.h $(INC)/sys/stream.h $(INC)/sys/strlog.h \
	$(INC)/sys/stropts.h $(INC)/sys/types.h

tcp_timer.o:	tcp_timer.c $(INC)/sys/errno.h $(INC)/sys/log.h \
	$(INC)/net/route.h \
	$(INC)/netinet/in.h \
	$(INC)/netinet/in_pcb.h $(INC)/netinet/in_systm.h \
	$(INC)/netinet/ip.h $(INC)/netinet/ip_var.h \
	$(INC)/netinet/tcp.h $(INC)/netinet/tcp_debug.h \
	$(INC)/netinet/tcp_fsm.h $(INC)/netinet/tcp_seq.h \
	$(INC)/netinet/tcp_timer.h $(INC)/netinet/tcp_var.h \
	$(INC)/netinet/tcpip.h $(INC)/sys/param.h \
	$(INC)/sys/socket.h $(INC)/sys/stream.h $(INC)/sys/strlog.h \
	$(INC)/sys/stropts.h $(INC)/sys/types.h

tcp_subr.o:	tcp_subr.c $(INC)/sys/cmn_err.h $(INC)/sys/errno.h \
	$(INC)/sys/log.h $(INC)/net/if.h $(INC)/net/if_arp.h \
	$(INC)/net/route.h $(INC)/netinet/in.h \
	$(INC)/netinet/in_pcb.h $(INC)/netinet/in_systm.h \
	$(INC)/netinet/in_var.h $(INC)/netinet/insrem.h \
	$(INC)/netinet/ip.h $(INC)/netinet/ip_icmp.h \
	$(INC)/netinet/ip_str.h $(INC)/netinet/ip_var.h \
	$(INC)/netinet/tcp.h $(INC)/netinet/tcp_fsm.h \
	$(INC)/netinet/tcp_seq.h $(INC)/netinet/tcp_timer.h \
	$(INC)/netinet/tcp_var.h $(INC)/netinet/tcpip.h \
	$(INC)/sys/param.h \
	$(INC)/sys/socket.h $(INC)/sys/stream.h \
	$(INC)/sys/strlog.h $(INC)/sys/stropts.h \
	$(INC)/sys/tihdr.h $(INC)/sys/types.h

clean:
		rm -f $(CLEAN)

clobber:	clean
		rm -f $(PRODUCTS)
