#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident "@(#)pump:pump.mk	1.14"
########
#
#	Standard Macros
#
########
AUX_CLEAN = 
CCLD1_CMD = $(CCLD_CMD) $(PPDEFS) $(CFLAGS) $(INC_LIST)
CCLD_CMD = $(CC) $(LDFLAGS)
CFLAGS = -s -O
DEFS = 
LDFLAGS = 
MAKE.LO = make.lo
MAKE.ROOT = ../../../../make.root
MKGEN = mkgen
PPDEFS = $(DEFS)
SYMLINK = :
INS = install
SGS = $(SGSX)
SGSX = 

INC_LIST	= -I$(INC)

CLEAN =

##########
#
#	make.root
#
##########

AUX_MACHINES=x86
SGSM32=m32
SGSX86=x86
INC=${ROOT}/usr/include

########
#
#	make.lo
#
########

LIB_LIST = -lld $(ROOTLIBS)
LD = $(SGS)ld

PRODUCTS = pump npump

all:	$(PRODUCTS)

install:	all
		-rm -f $(ROOT)/etc/pump
		-rm -f $(ROOT)/etc/npump
		$(INS) -f $(ROOT)/sbin -m 0544 -u root -g sys pump
		$(INS) -f $(ROOT)/usr/sbin -m 0544 -u root -g sys pump
		$(INS) -f $(ROOT)/sbin  -m 0544 -u root -g sys  npump
		$(INS) -f $(ROOT)/usr/sbin -m 0544 -u root -g sys npump
		-$(SYMLINK) /sbin/pump $(ROOT)/etc/pump
		-$(SYMLINK) /sbin/npump $(ROOT)/etc/npump

########
#
#	All dependencies and rules not explicitly stated
#	(including header and nested header dependencies)
#
########

pump:	pump.c
pump:	$(INC)/stdio.h
pump:	$(INC)/string.h
pump:	$(INC)/fcntl.h
pump:	$(INC)/sys/types.h
pump:	$(INC)/sys/stat.h
pump:	$(INC)/sys/mkdev.h
pump:	$(INC)/filehdr.h
pump:	$(INC)/scnhdr.h
pump:	$(INC)/ldfcn.h
pump:	$(INC)/sys/cio_defs.h
pump:	$(INC)/sys/pump.h
	$(CCLD1_CMD) -D_STYPES -o pump pump.c $(LIB_LIST)

npump:	npump.c
npump:	$(INC)/stdio.h
npump:	$(INC)/string.h
npump:	$(INC)/fcntl.h
npump:	$(INC)/sys/types.h
npump:	$(INC)/filehdr.h
npump:	$(INC)/scnhdr.h
npump:	$(INC)/ldfcn.h
npump:	$(INC)/sys/stropts.h
npump:	$(INC)/sys/strpump.h
	$(CCLD1_CMD) -o npump npump.c $(LIB_LIST)

########
#	Standard Targets
#
#	all		builds all the products specified by PRODUCTS
#	clean		removes all temporary files (ex. installable object)
#	clobber		"cleans", and then removes $(PRODUCTS)
#	makefile	regenerates makefile
#	install		installs products; user defined in make.lo 
#
########

clean:
		rm -f $(CLEAN) $(AUX_CLEAN)

clobber:	clean
		rm -f $(PRODUCTS)

makefile:	$(MAKE.LO) $(MAKE.ROOT)
		$(MKGEN) >make.out
		if [ -s make.out ]; then mv make.out makefile; fi

makefile_all:	makefile
