#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident "@(#)ports:ports.mk	1.5.5.1"
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
MAKE.ROOT = $(ROOT)/make.root
MKGEN = mkgen
SYMLINK = :
INS = install
PPDEFS = $(DEFS)

INC_LIST	= -I$(INC)

CLEAN =

##########
#
#	make.root
#
##########

# define permissible SGS's
AUX_MACHINES=m32;u3b2,m32 x86 b16
SGSM32=m32
SGSX86=x86
SGSB16=b16

# define UNIX header directory
INC=${ROOT}/usr/include

# define rest of SGS (CC, LD, AS, DIS are handled by mkgen)
CKFILE=$(SGS)ckfile
CONV=$(SGS)conv
DUMP=$(SGS)dump
LIST=$(SGS)list
LORDER=$(SGS)lorder
NM=$(SGS)nm
SIZE=$(SGS)size

########
#
#	make.lo
#
########

SGS=m32
MACHINE=3b2
PRODUCTS=ports

########
#
#	All dependencies and rules not explicitly stated
#	(including header and nested header dependencies)
#
########

ports:	ports.c
ports:	$(INC)/stdio.h
ports:	$(INC)/fcntl.h
ports:	$(INC)/string.h
ports:	$(INC)/grp.h
ports:	$(INC)/sac.h
ports:	$(INC)/sys/sysmacros.h
ports:	$(INC)/sys/edt.h
ports:	$(INC)/sys/sys3b.h
ports:	$(INC)/sys/types.h
ports:	$(INC)/sys/stat.h
ports:	$(INC)/dirent.h
ports:	$(INC)/sys/cio_defs.h
ports:	$(INC)/sys/pp_dep.h
ports:	$(INC)/sys/queue.h
ports:	$(INC)/sys/stream.h
ports:	$(INC)/sys/stropts.h
ports:	$(INC)/sys/strpump.h
ports:	$(INC)/sys/strppc.h
	$(CCLD1_CMD) -o ports ports.c $(LIB_LIST) $(ROOTLIBS)

install:	all
	-rm -f $(ROOT)/etc/ports
	$(INS) -f $(ROOT)/usr/sbin -m 0555 -u bin -g bin ports
	$(INS) -f $(ROOT)/sbin -m 0555 -u bin -g bin ports
	-$(SYMLINK) $(ROOT)/usr/sbin/ports $(ROOT)/etc/ports

########
#
#	Standard Targets
#
#	all		builds all the products specified by PRODUCTS
#	clean		removes all temporary files (ex. installable object)
#	clobber		"cleans", and then removes $(PRODUCTS)
#	makefile	regenerates makefile
#	install		installs products; user defined in make.lo 
#
########

all:		$(PRODUCTS)

clean:

clobber:	clean
		rm -f $(PRODUCTS)

makefile:	$(MAKE.LO) $(MAKE.ROOT)
		$(MKGEN) >make.out
		if [ -s make.out ]; then mv make.out makefile; fi

makefile_all:	makefile

