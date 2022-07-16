#	Copyright (c) 1988 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libm:m32_sfm/makefile	1.4"
#
# makefile for libsfm
#
# Note that the object list given in SOURCES is in the order in which the files
# should be added to the archive when it is created.  If the files are not
# ordered correctly, linking of libm fails on the pdp11 and is slowed down
# on other machines.

ROOT=
SGS=
OWN=bin
GRP=bin
CFLAGS=-O
LDLIBS=
LINT=lint
LINTFLAGS=
SGSBASE=../../../cmd/sgs
INS=$(SGSBASE)/sgs.install
INSDIR=
STRIP=strip
HFILES=../port/fpparts.h
PRODUCTS=
CCSLIB=$(ROOT)/usr/ccs/lib
PROF=
NONPROF=
DEFLIST=
MAKE=make
INSPDIR=
AR=ar
LORDER=lorder
PORT=../port
LIBSA = libsfm.sa
M4DEF=../m32mau/m4.def
AR=ar

SOURCES =\
	sinf.m\
	cosf.m\
	tanf.m\
	atanf.m\
	logf.m\
	log10f.m\
	expf.m\
	sqrtf.m

all: spreproc 

nonprofarch: all $(SOURCES)
	$(AR) r $(LIBSA) *.s

profarch: all $(SOURCES)
	$(AR) r $(LIBSA) *.s

archive: all $(SOURCES)
	$(AR) r $(LIBSA) *.s

.c.a:;

spreproc: $(SOURCES)
		for i in $(SOURCES:.m=); do\
			m4 $(M4DEF) -DMCOUNT=# $$i.m > $$i.s; \
		done;

move:
	#
	# move the library into the correct directory
	sh $(INS) 644 $(OWN) $(GRP) $(INSDIR)/$(LIBSA) $(LIBSA)

clean:
	-rm -f *.s

clobber:	clean
	-rm -f $(LIBSA)
