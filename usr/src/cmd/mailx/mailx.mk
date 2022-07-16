#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)mailx:mailx.mk	1.17.4.2"
# "@(#)mx.mk	1.4 'attmail mail(1) command'"
#
# mailx -- a modified version of a University of California at Berkeley
#	mail program
#
# for standard Unix
#

ROOT=
VERSION=4.0
HDR=	hdr
HELP=	help
DESTDIR= $(ROOT)/usr/bin

# If system == SVR3, use the following...
#DESTLIB = $(ROOT)/usr/lib/mailx
#CPPDEFS = -DpreSVr4 -I$(HDR) -I$(CRX)/usr/include -I$(ROOT)/usr/include 
#CFLAGS  = -O $(CPPDEFS) 
#LDFLAGS = -s
#LD_LIBS = -L$(ROOT)/usr/lib -lmail
#SYMLINK = :

# If system == SVR4, use the following...
DESTLIB = $(ROOT)/usr/share/lib/mailx
CPPDEFS = -I$(HDR) -I$(CRX)/usr/include -I$(ROOT)/usr/include 
CFLAGS  = -O -v -Xa  $(CPPDEFS) 
LD_FLAGS = -s $(LDFLAGS) $(PERFLIBS)
LD_LIBS = -L$(ROOT)/usr/lib -lmail $(LDLIBS)
SYMLINK = ln -s

MAILDIR = $(ROOT)/usr/mail

HOSTCC=	/bin/cc
INS=	install

SRCS=	myfopen.c aux.c cmd1.c cmd2.c cmd3.c cmd4.c cmdtab.c collect.c\
	config.c edit.c \
	init.c is.c fio.c getname.c head.c hostname.c lex.c \
	list.c lock.c\
	lpaths.c main.c names.c optim.c popen.c quit.c receipt.c send.c \
	sigretro.c stralloc.c temp.c translate.c tty.c usg.local.c vars.c \
	version.c

OBJS=	$(SRCS:.c=.o)

HDRS=	$(HDR)/configdefs.h \
	$(HDR)/def.h \
	$(HDR)/glob.h \
	$(HDR)/local.h \
	$(HDR)/rcv.h \
	$(HDR)/sigretro.h \
	$(HDR)/uparm.h \
	$(HDR)/usg.local.h

S=	$(SRCS) version.c $(HDRS)

.c.o:
	$(CC) -c $(CFLAGS) $*.c

all:	mailx

mailx:	$S $(OBJS)
	-rm -f mailx
	$(CC) $(LD_FLAGS) -o mailx $(OBJS) $(LD_LIBS)

install: ckdirs all
	$(INS) -f $(DESTDIR) -m 2511 -g mail -u bin mailx
	grep -v '^#.*@(' $(HELP)/mailx.help > /tmp/mailx.help
	$(INS) -f $(DESTLIB) -m 644 -u bin -g bin /tmp/mailx.help
	rm /tmp/mailx.help
	grep -v '^#.*@(' $(HELP)/mailx.help.~ > /tmp/mailx.help.~
	$(INS) -f $(DESTLIB) -m 644 -u bin -g bin /tmp/mailx.help.~
	rm /tmp/mailx.help.~

version.o:	mailx.mk version.c
	$(CC) -c version.c

version.c:
	echo \
	"char *version=\"mailx version $(VERSION)\";"\
		> version.c

clean:
	-rm -f *.o
	-rm -f version.c a.out core 

clobber:	clean
	-rm -f mailx

lint:	version.c
	$(PFX)lint $(CFLAGS) $(SRCS)

mailx.cpio:	$(SRCS) $(HDRS) mailx.mk 
	@echo $(SRCS) $(HDRS) mailx.mk | \
		tr " " "\012" | \
		cpio -oc >mailx.cpio

listing:
	pr mailx.mk hdr/*.h [a-l]*.c | lp
	pr [m-z]*.c | lp

ckdirs:
	if [ ! -d $(DESTLIB) ] ; then mkdir $(DESTLIB) ; fi
	$(SYMLINK) $(DESTLIB) /usr/lib/mailx

chgrp: 
	chgrp mail mailx
	chmod g+s mailx
