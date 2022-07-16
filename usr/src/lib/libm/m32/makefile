#	Copyright (c) 1988 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libm:m32/makefile	1.10"
#
# makefile for libm/m32
#
#
#

.SUFFIXES: .p

ROOT=
SGS=
OWN=bin
GRP=bin
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
DEFLIST=-DM32
MAKE=make
INSPDIR=
CMODE=-Kfpe
INCMATH=../port
M32MATH=../m32
INCLIST=-I$(INCMATH)
AR=ar
LORDER=lorder
PORT=../port


OBJECTS=\
$(PORT)/acosh.o	$(PORT)/asin.o	$(PORT)/asinh.o		\
$(PORT)/atanh.o	$(PORT)/cbrt.o	$(PORT)/erf.o	\
$(PORT)/fabs.o	$(PORT)/floor.o	$(PORT)/fmod.o	\
$(PORT)/gamma.o	$(PORT)/hypot.o $(PORT)/sqrt.o  \
$(PORT)/j0.o	$(PORT)/j1.o	$(PORT)/jn.o	\
$(PORT)/log1p.o $(PORT)/matherr.o $(PORT)/rint.o	\
$(PORT)/pow.o	$(PORT)/rem.o	$(PORT)/sinh.o  $(PORT)/tanh.o \
$(PORT)/asinf.o	$(PORT)/fabsf.o	\
$(PORT)/floorf.o	$(PORT)/fmodf.o	\
$(PORT)/powf.o	$(PORT)/sinhf.o	 \
$(PORT)/tanhf.o \
$(PORT)/atan.o $(PORT)/exp.o $(PORT)/log.o $(PORT)/reduce.o \
$(PORT)/sin.o $(PORT)/atanf.o $(PORT)/expf.o $(PORT)/logf.o \
$(PORT)/reducef.o $(PORT)/sinf.o $(PORT)/sqrtf.o


POBJECTS=\
$(PORT)/acosh.p	$(PORT)/asin.p	$(PORT)/asinh.p		\
$(PORT)/atanh.p	$(PORT)/cbrt.p	$(PORT)/erf.p	\
$(PORT)/fabs.p	$(PORT)/floor.p	$(PORT)/fmod.p	\
$(PORT)/gamma.p	$(PORT)/hypot.p  $(PORT)/sqrt.p \
$(PORT)/j0.p	$(PORT)/j1.p	$(PORT)/jn.p	\
$(PORT)/log1p.p $(PORT)/matherr.p $(PORT)/rint.p	\
$(PORT)/pow.p	$(PORT)/rem.p	$(PORT)/sinh.p  $(PORT)/tanh.p \
$(PORT)/asinf.p	$(PORT)/fabsf.p	\
$(PORT)/floorf.p	$(PORT)/fmodf.p	\
$(PORT)/powf.p	$(PORT)/sinhf.p	 $(PORT)/sqrtf.p \
$(PORT)/tanhf.p \
$(PORT)/atan.p $(PORT)/exp.p $(PORT)/log.p $(PORT)/reduce.p \
$(PORT)/sin.p $(PORT)/atanf.p $(PORT)/expf.p $(PORT)/logf.p \
$(PORT)/reducef.p $(PORT)/sinf.p


.c.o:
	@echo $*.c:
	$(CC) $(DEFLIST) $(INCLIST) $(CFLAGS) $(CMODE)  -c $*.c

.c.p:
	@echo $*.c:
	$(CC) $(DEFLIST) $(INCLIST) $(CFLAGS) $(CMODE) -c -p $*.c  && mv $(*F).o $(*F).p


all:	prof nonprof


nonprof: $(OBJECTS)

prof:  $(POBJECTS)

$(OBJECTS):	$(HFILES)

$(POBJECTS):	$(HFILES)


archive:	all nonprofarch profarch
	#
	#
nonprofarch:	$(OBJECTS)
	#
	$(LORDER) *.o | tsort >objlist 
	# build the archive with the modules in correct order.
	$(AR) q lib.libm `cat objlist` 
	#
profarch:	$(POBJECTS)
	# build the profiled library archive, first renaming the
	# .p (profiled object) modules to .o
	for i in *.p ; do mv $$i `basename $$i .p`.o ; done
	$(LORDER) *.o | tsort >objlist 
	$(AR) q libp.libm `cat objlist` 

	#
	#

move:	nonprofmove profmove
	#

nonprofmove:
	# move the library or libraries into the correct directory
	sh $(INS) 644 $(OWN) $(GRP) $(INSDIR)/libm.a lib.libm ; \
	rm -f lib.libm
profmove:
	if [ ! -d $(INSPDIR) ]; then \
	mkdir $(INSPDIR); \
	fi
	sh $(INS) 644 $(OWN) $(GRP) $(INSPDIR)/libm.a libp.libm ; \
	# delete temporary libraries
	-rm -f libp.libm

clean:
	-rm -rf *.[op] tmp
	-rm -rf lib*.contents objlist

clobber: clean
	-rm -rf *.[op] tmp
	-rm -rf lib*.libm lib*.contents objlist

