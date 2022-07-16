#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)nfs.cmds:nfs/lockd/lockd.mk	1.10"

BINS= lockd
OBJS= xdr_klm.o xdr_nlm.o xdr_sm.o prot_pklm.o prot_msg.o prot_lock.o \
      prot_alloc.o prot_free.o prot_share.o hash.o ufs_lockf.o \
      tcp.o prot_proc.o prot_libr.o signal.o prot_pnlm.o \
      prot_priv.o  sm_monitor.o prot_main.o setbuffer.o \
      port.o getdname.o rpc.o svc_create.o svc_dg.o ti_opts.o \
      flk_filock.o flk_reclox.o

# pmap_clnt.o pmap_prot.o rpc_soc.o svc.o

SRCS= $(OBJS:.o=.c)
INC = $(ROOT)/usr/include
INCSYS = $(ROOT)/usr/include
INS = install
INSDIR = $(ROOT)/usr/lib/nfs

CPPFLAGS= 
CFLAGS= $(CPPFLAGS) -I$(INC)
LINTFLAGS= -hbax $(CPPFLAGS)
LDFLAGS = -s
COFFLIBS= -lrpc -ldes -lnsl_s -lsocket
#ELFLIBS = -lrpcsvc -dy -lnsl -lrpc -ldes -lnet -lsocket
ELFLIBS = -dy -lnsl 

all: $(BINS)

$(BINS): $(OBJS)
	if [ x$(CCSTYPE) = xCOFF ] ; \
	then	$(CC) -o $@ $(OBJS) $(LDFLAGS) $(LIBOPT) $(SHLIBS) $(COFFLIBS) ; \
	else	$(CC) -o $@ $(OBJS) $(LDFLAGS) $(LIBOPT) $(SHLIBS) $(ELFLIBS) ; \
	fi

install: all
	if [ ! -d $(INSDIR) ] ; \
	then mkdir -p $(INSDIR); \
	fi
	$(INS) -f $(INSDIR) -m 0555 -u bin -g bin $(BINS)

lint:
	lint $(LINTFLAGS) $(SRCS)

tags: $(SRCS)
	ctags -tw $(SRCS)

clean:
	-rm -f $(OBJS)

clobber: clean
	-rm -f $(BINS)
