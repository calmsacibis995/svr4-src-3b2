#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libpkg:libpkg.mk	1.13.1.10"
.c.a:;

INC=$(ROOT)/usr/include
INCSYS=$(ROOT)/usr/include/sys
USRLIB=$(ROOT)/usr/lib
LIBPKG=libpkg.a
INSTALL=install
LINTLIBPKG=llib-lpkg.ln
LINTFILES=srchcfile.c putcfile.c \
	gpkgmap.c ppkgmap.c tputcfent.c \
	verify.c cvtpath.c mappath.c \
	canonize.c logerr.c progerr.c \
	dstream.c pkgtrans.c \
	gpkglist.c isdir.c runcmd.c \
	rrmdir.c ckvolseq.c devtype.c \
	pkgmount.c pkgexecv.c pkgexecl.c 

CFLAGS=-O -I $(INC)

PKGINFO_FILES= \
	$(LIBPKG)(srchcfile.o) $(LIBPKG)(putcfile.o) \
	$(LIBPKG)(gpkgmap.o) $(LIBPKG)(ppkgmap.o) $(LIBPKG)(tputcfent.o)\
	$(LIBPKG)(verify.o) $(LIBPKG)(cvtpath.o) $(LIBPKG)(mappath.o) \
	$(LIBPKG)(canonize.o) $(LIBPKG)(logerr.o) $(LIBPKG)(progerr.o) \
	$(LIBPKG)(dstream.o) $(LIBPKG)(pkgtrans.o) \
	$(LIBPKG)(gpkglist.o) $(LIBPKG)(isdir.o) $(LIBPKG)(runcmd.o) \
	$(LIBPKG)(rrmdir.o) $(LIBPKG)(ckvolseq.o) $(LIBPKG)(devtype.o) \
	$(LIBPKG)(pkgmount.o) \
	$(LIBPKG)(pkgexecv.o) $(LIBPKG)(pkgexecl.o) 

all:	$(LIBPKG)

.PRECIOUS: $(LIBPKG)

$(LIBPKG): $(PKGINFO_FILES) 
	$(CC) -c $(CFLAGS) $(PRESVR4) $(?:.o=.c)
	$(AR) rv $(LIBPKG) $?
	rm $?

$(PKGINFO_FILES): $(INC)/pkginfo.h $(INC)/pkgstrct.h

clean:
	rm -f lint.out llib-libpkg.ln

clobber: clean
	rm -f $(LIBPKG)

strip:

install: all
	$(INSTALL) -f $(USRLIB) $(LIBPKG)
	@if [ -f $(LINTLIBPKG) ] ;\
	then \
		$(INSTALL) -f $(ROOT)/usr/lib $(LINTLIBPKG) ;\
	fi

lintit:
	lint -I $(INC) -u -o pkg $(LINTFILES) > lint.out 2>&1
