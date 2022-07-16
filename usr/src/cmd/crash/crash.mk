#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)crash-3b2:crash.mk	1.11.6.1"
STRIP = 
DBO = -DDBO
MAKE = make "AS=$(AS)" "CC=$(CC)" "LD=$(LD)"

TESTDIR = .
ROOT = 
INSDIR = $(ROOT)/usr/sbin
INS = install
LDFLAGS = -s
INC=$(ROOT)/usr/include
SYMLINK = :
INCSRC=$(ROOT)/usr/src/uts/3b2
COMFLAGS = -D_KMEMUSER -I${INC} -I. -Uvax -Uu3b -Updp11 -Uu3b15 -Du3b2 $(DBO)
CFLAGS= $(COMFLAGS)
LIBELF = -lelf
FRC =

OFILES= async.o \
	base.o \
	buf.o \
	callout.o \
	class.o \
	dis.o \
	disp.o \
	events.o \
	init.o \
	inode.o \
	kma.o \
	lck.o \
	main.o \
	major.o \
	map.o \
	nvram.o \
	page.o \
	prnode.o \
	proc.o \
	pty.o \
	resource.o \
	rfs.o \
	rt.o \
	search.o \
	size.o \
	sizenet.o \
	snode.o \
	stream.o \
	syms.o \
	symtab.o \
	ts.o \
	tty.o \
	u.o \
	ufs_inode.o \
	util.o \
	var.o \
	vfs.o \
	vfssw.o \
	vtop.o 

CFILES= async.c \
	base.c \
	buf.c \
	callout.c \
	class.c \
	dis.c \
	disp.c \
	events.c \
	init.c \
	inode.c \
	kma.c \
	lck.c \
	main.c \
	major.c \
	map.c \
	nvram.c \
	page.c \
	prnode.c \
	proc.c \
	resource.c \
	rfs.c \
	rt.c \
	search.c \
	size.c \
	sizenet.c \
	snode.c \
	stream.c \
	syms.s \
	symtab.c \
	ts.c \
	tty.c \
	u.c \
	util.c \
	var.c \
	vfs.c \
	vfssw.c \
	vtop.c 


all: crash ldsysdump

crash:	$(OFILES)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TESTDIR)/crash $(OFILES) $(LIBELF) $(SHLIBS)

ldsysdump: ldsysdump.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TESTDIR)/ldsysdump ldsysdump.c $(SHLIBS)

install: ins_crash ins_ldsysdump

ins_crash: crash
	-rm -f $(ROOT)/etc/crash
	$(INS) -f $(INSDIR) -m 0555 -u bin -g bin $(TESTDIR)/crash
	-$(SYMLINK) /usr/sbin/crash $(ROOT)/etc/crash

ins_ldsysdump: ldsysdump
	-rm -f $(ROOT)/etc/ldsysdump
	$(INS) -f $(INSDIR) -m 0555 -u bin -g bin $(TESTDIR)/ldsysdump
	-$(SYMLINK) /usr/sbin/ldsysdump $(ROOT)/etc/ldsysdump

clean:
	-rm -f *.o

clobber: clean
	-rm -f $(TESTDIR)/crash
	-rm -f $(TESTDIR)/ldsysdump

xref: $(CFILES) $(HFILES) 
	cxref -c $(COMFLAGS) $(CFILES) | pr -h crash.cxref | opr

lint: $(CFILES) $(HFILES) 
	lint $(COMFLAGS) -uh $(CFILES) 

prall:
	pr -n $(CFILES) | opr
	pr -n $(HFILES) | opr

FRC:
