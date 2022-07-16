#	Copyright (c) 1988 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)xenv:m32/template.mk	1.83"
#
#	WE-32000 GLOBAL MAKEFILE
#
#
#	SGS indicates the prefix to be applied to the installed commands.
SGS=
#
#	ARCH indicates the architecture of the host machine
#		AR16WR=11/70, AR32WR=vax, AR32W=ibm, 3b20, etc
ARCH=AR32W
#
#	The following macros define the various installation directories.
#	Note that if relative pathnames are used, they must allow for the
#	"cd" issued prior to each make.
#
#	SGSBASE is the directory under which all the sgs source lives
SGSBASE= ../..
#
#	CCSBIN is the directory for installing executable ("bin") files.
#	It should correspond to the same directory as specified in AS and LD
#	in paths.h.
CCSBIN=/usr/ccs/bin
#
#	CCSLIB is the directory for installing libraries and executable
#	files not intended for direct user execution (e.g., assembler pass1).
#	It should correspond to the same directory as specified in COMP,
#	OPTIM, AS1, and AS2 in paths.h.
CCSLIB=/usr/ccs/lib
ETC=/etc
#
#	Specify the byte order for this SGS instance.
#		FBO = Forward Byte Order (3B20, IBM)
#		RBO = Reverse Byte Order (DEC)
DBO=FBO
#
#	If host machine is running a pre-5.0 release of UNIX
#	then set Archive Format to "OLDAR".
#	Starting with the 5.0 UNIX (SVR1)  release the Archive Format
#	should be set to "PORTAR".
#	If building a 5.0 release on a host that is running
#	System V Rel. 2.0 (SVR2), then set ARFORMAT to PORT5AR.
#
ARFORMAT=PORTAR
#
#	Starting with the SVR2 release of UNIX,
#	if flexnames feature is desired, then set FLEX
#	to -DFLEXNAMES.
#	If host machine is running a pre-SVR2 release of UNIX
#	then set FLEX to null (ie. FLEX= ).
#
FLEX=-DFLEXNAMES
#
#	MAC parameter specifies the set of software workarounds
#	to be produced by the new sgs.
#	MAC=ABWORMAC produces sgs for WE32001 and WE32100 chips.
#	MAC=BMAC produces sgs for WE32100  o n l y.
#	MAC=BMAUMAC produces sgs for WE32100 with WE32106 only.
#
MAC=BMAC
#	This is the machine ID field. The valid values are
#	i386, m32, sparc
MACH=m32
MNAME=u3b2
#
#	The following parameter specifies the default include directory
#	for cpp. If not defined the value will be ROOT/usr/include.
#
NATIVE=yes
INC=/usr/include
INCSYS=/usr/include
DFLTINC=/usr/include
NOUSRINC=NOUSRINC
#
#
OWN=bin
GRP=bin
#
#
MAKE=make
YACC=yacc
LEX=lex
CC=cc
AR=ar
CFLAGS=-c
FFLAG=
ENV=
ROOT=
VERSION=
LDLIBS=
LPASS=.
MACHINC=$(SGSBASE)/inc/m32
LIBELF=$(SGSBASE)/libelf/m32/libelf.a
LIBLD=$(SGSBASE)/libld/m32/libld.a

# The CCS by default produces dynamically-linked executables.
# By setting LINK_MODE to -dn, dynamic linking is turned off.
LINK_MODE=-dn

#
#
ENVPARMS=MAKE="$(MAKE)" SGS="$(SGS)" ARCH="$(ARCH)" OWN="$(OWN)" GRP="$(GRP)" \
	DBO="$(DBO)" ARFORMAT="$(ARFORMAT)" FLEX="$(FLEX)" LDLIBS="$(LDLIBS)" \
	YACC="$(YACC)" LEX="$(LEX)" LIBLD="$(LIBLD)" LIBELF="$(LIBELF)"\
	MACH="$(MACH)" MACHINC="$(MACHINC)" NATIVE="$(NATIVE)"

CPPARMS=CC="$(CC)" FFLAG="$(FFLAG)" ENV="$(ENV)" ROOT="$(ROOT)" \
	VERSION="$(VERSION)" INC="$(INC)" DFLTINC="$(DFLTINC)"

CCPARMS=CC="$(CC)" FFLAG="$(FFLAG)" ENV="$(ENV)" ROOT="$(ROOT)" \
	VERSION="$(VERSION)" USRINC="$(NOUSRINC)"

DIRPARMS=CCSBIN="$(CCSBIN)" CCSLIB="$(CCSLIB)" ETC="$(ETC)"
LINKPARMS=LINK_MODE="$(LINK_MODE)"
#
#
all:	tools libs
	cd $(SGSBASE)/ar/m32; $(MAKE) $(ENVPARMS) $(CCPARMS)
	cd $(SGSBASE)/acpp/m32; $(MAKE) $(ENVPARMS) $(CPPARMS) clean
	cd $(SGSBASE)/acomp/m32; $(MAKE) $(ENVPARMS) $(CPPARMS) $(LINKPARMS)
	cd $(SGSBASE)/acomp/m32mau; $(MAKE) $(ENVPARMS) $(CPPARMS) $(LINKPARMS)
	cd $(SGSBASE)/acpp/m32; $(MAKE) $(ENVPARMS) $(CPPARMS) clean
	cd $(SGSBASE)/alint/m32; $(MAKE) $(ENVPARMS) $(CPPARMS) LPASS=$(LPASS)
	cd $(SGSBASE)/cpp/m32; $(MAKE) $(ENVPARMS) $(CPPARMS) PD_SYS=D_unix \
		PD_MACH=D_newmach NEW_MACH="-DPD_MY_MACH=\\\"$(MNAME)\\\""
	cd $(SGSBASE)/cmd/m32; $(MAKE) $(ENVPARMS) $(CCPARMS) MAC=$(MAC) $(LINKPARMS)
	cd $(SGSBASE)/newoptim/m32; $(MAKE) $(ENVPARMS) $(CCPARMS) MAC=$(MAC)
	cd $(SGSBASE)/as/m32; $(MAKE) $(ENVPARMS) $(CCPARMS) MAC=$(MAC) $(LINKPARMS)
	cd $(SGSBASE)/ld/m32; $(MAKE) $(ENVPARMS) $(CCPARMS) $(LINKPARMS)
	cd $(SGSBASE)/dis/m32; $(MAKE) $(ENVPARMS) $(CCPARMS)
	cd $(SGSBASE)/dump/m32; $(MAKE) $(ENVPARMS) $(CCPARMS)
	if [ X"$(NATIVE)" = Xyes ] ; \
	then \
		if [ "$(MNAME)" = "u3b2" ] ; \
		then \
			cd $(SGSBASE)/hdwr_tst/m32; $(MAKE) $(ENVPARMS) $(CCPARMS) ; \
		fi \
	fi
	cd $(SGSBASE)/cof2elf/m32; $(MAKE) $(ENVPARMS) $(CCPARMS)
	cd $(SGSBASE)/lorder/m32; $(MAKE) $(ENVPARMS)
	cd $(SGSBASE)/lprof/m32; $(MAKE) $(ENVPARMS)
	cd $(SGSBASE)/m4/m32; $(MAKE) $(ENVPARMS) $(CCPARMS)
	cd $(SGSBASE)/mcs/m32; $(MAKE) $(ENVPARMS) $(CCPARMS)
	cd $(SGSBASE)/nm/m32; $(MAKE) $(ENVPARMS) $(CCPARMS)
	cd $(SGSBASE)/size/m32; $(MAKE) $(ENVPARMS) $(CCPARMS)
	cd $(SGSBASE)/strip/m32; $(MAKE) $(ENVPARMS) $(CCPARMS)
	cd $(SGSBASE)/tsort/m32; $(MAKE) $(ENVPARMS) $(CCPARMS)
	echo "Successfully built tools."
#
tools:
	cd $(SGSBASE)/yacc/m32; $(MAKE) $(ENVPARMS) $(CCPARMS) $(DIRPARMS)
	cd $(SGSBASE)/lex/m32; $(MAKE) $(ENVPARMS) $(CCPARMS) $(DIRPARMS)
#
libs:
	cd $(SGSBASE)/libelf/m32; $(MAKE) $(ENVPARMS) $(CCPARMS)
	cd $(SGSBASE)/libld/m32; $(MAKE) $(ENVPARMS) $(CCPARMS)
#
install: toolinstall libinstall
	cd $(SGSBASE)/ar/m32; $(MAKE) $(ENVPARMS) $(CCPARMS) $(DIRPARMS) install
	cd $(SGSBASE)/acomp/m32mau; $(MAKE) $(ENVPARMS) $(CPPARMS) $(DIRPARMS) install MAC=$(MAC) $(LINKPARMS)
	cd $(SGSBASE)/acomp/m32; $(MAKE) $(ENVPARMS) $(CPPARMS) $(DIRPARMS) install MAC=$(MAC) $(LINKPARMS)
	cd $(SGSBASE)/alint/m32; $(MAKE) $(ENVPARMS) $(CPPARMS) $(DIRPARMS) LPASS=$(LPASS) install 
	cd $(SGSBASE)/cpp/m32; $(MAKE) $(ENVPARMS) $(CPPARMS) $(DIRPARMS) install \
		PD_SYS=D_unix PD_MACH=D_newmach NEW_MACH="-DPD_MY_MACH=\\\"$(MNAME)\\\""
	cd $(SGSBASE)/cmd/m32; $(MAKE) $(ENVPARMS) $(CCPARMS) $(DIRPARMS) install MAC=$(MAC) $(LINKPARMS)
	cd $(SGSBASE)/newoptim/m32; $(MAKE) $(ENVPARMS) $(CCPARMS) $(DIRPARMS) install
	cd $(SGSBASE)/as/m32; $(MAKE) $(ENVPARMS) $(CCPARMS) $(DIRPARMS) install MAC=$(MAC) $(LINKPARMS)
	cd $(SGSBASE)/ld/m32; $(MAKE) $(ENVPARMS) $(CCPARMS) $(DIRPARMS) install $(LINKPARMS)
	cd $(SGSBASE)/dis/m32; $(MAKE) $(ENVPARMS) $(CCPARMS) $(DIRPARMS) install
	cd $(SGSBASE)/dump/m32; $(MAKE) $(ENVPARMS) $(CCPARMS) $(DIRPARMS) install
	if [ X"$(NATIVE)" = Xyes ] ; \
	then \
		if [ "$(MNAME)" = "u3b2" ] ; \
		then \
			cd $(SGSBASE)/hdwr_tst/m32; $(MAKE) $(ENVPARMS) $(CCPARMS) $(DIRPARMS) install ; \
		fi \
	fi
	cd $(SGSBASE)/cof2elf/m32; $(MAKE) $(ENVPARMS) $(CCPARMS) $(DIRPARMS) install
	cd $(SGSBASE)/lorder/m32; $(MAKE) $(ENVPARMS) $(DIRPARMS) install
	cd $(SGSBASE)/lprof/m32; $(MAKE) $(ENVPARMS) $(DIRPARMS) install
	cd $(SGSBASE)/m4/m32; $(MAKE) $(ENVPARMS) $(CCPARMS) $(DIRPARMS) install
	cd $(SGSBASE)/mcs/m32; $(MAKE) $(ENVPARMS) $(CCPARMS) $(DIRPARMS) install
	cd $(SGSBASE)/nm/m32; $(MAKE) $(ENVPARMS) $(CCPARMS) $(DIRPARMS) install
	cd $(SGSBASE)/size/m32; $(MAKE) $(ENVPARMS) $(CCPARMS) $(DIRPARMS) install
	cd $(SGSBASE)/strip/m32; $(MAKE) $(ENVPARMS) $(CCPARMS) $(DIRPARMS) install
	cd $(SGSBASE)/tsort/m32; $(MAKE) $(ENVPARMS) $(CCPARMS) $(DIRPARMS) install
	echo "Successfully installed tools.\n"
#
toolinstall:
	cd $(SGSBASE)/yacc/m32; $(MAKE) $(ENVPARMS) $(CCPARMS) $(DIRPARMS) install
	cd $(SGSBASE)/lex/m32; $(MAKE) $(ENVPARMS) $(CCPARMS) $(DIRPARMS) install
#
libinstall:
	cd $(SGSBASE)/libelf/m32; $(MAKE) $(ENVPARMS) $(CCPARMS) $(DIRPARMS) install
	cd $(SGSBASE)/libld/m32; $(MAKE) $(ENVPARMS) $(CCPARMS) $(DIRPARMS) install
#
libcopy:
	# save host version of libelf
	if [ -n "$(SGS)" ] ;\
	then \
		mv $(CCSLIB)/libelf.a $(CCSLIB)/libelf$(SGS).a;\
	fi
#
lintit: toollintit liblintit
	cd $(SGSBASE)/ar/m32; $(MAKE) $(ENVPARMS) $(CCPARMS) lintit
	cd $(SGSBASE)/acomp/m32mau; $(MAKE) $(ENVPARMS) lintit
	cd $(SGSBASE)/acomp/m32; $(MAKE) $(ENVPARMS) lintit
	cd $(SGSBASE)/cpp/m32; $(MAKE) $(ENVPARMS) lintit
	cd $(SGSBASE)/cmd/m32; $(MAKE) $(ENVPARMS) lintit
	cd $(SGSBASE)/newoptim/m32; $(MAKE) $(ENVPARMS) lintit
	cd $(SGSBASE)/as/m32; $(MAKE) $(ENVPARMS) lintit
	cd $(SGSBASE)/ld/m32; $(MAKE) $(ENVPARMS) lintit
	cd $(SGSBASE)/dis/m32; $(MAKE) $(ENVPARMS) lintit
	cd $(SGSBASE)/dump/m32; $(MAKE) $(ENVPARMS) lintit
	if [ X"$(NATIVE)" = Xyes ] ; \
	then \
		if [ "$(MNAME)" = "u3b2" ] ; \
		then \
			cd $(SGSBASE)/hdwr_tst/m32; $(MAKE) $(ENVPARMS) lintit ; \
		fi \
	fi
	cd $(SGSBASE)/alint/m32; $(MAKE) $(ENVPARMS) lintit
	cd $(SGSBASE)/cof2elf/m32; $(MAKE) $(ENVPARMS) lintit
	cd $(SGSBASE)/lorder/m32; $(MAKE) $(ENVPARMS) lintit
	cd $(SGSBASE)/lprof/m32; $(MAKE) $(ENVPARMS) lintit
	cd $(SGSBASE)/m4/m32; $(MAKE) $(ENVPARMS) lintit 
	cd $(SGSBASE)/mcs/m32; $(MAKE) $(ENVPARMS) lintit 
	cd $(SGSBASE)/nm/m32; $(MAKE) $(ENVPARMS) lintit
	cd $(SGSBASE)/size/m32; $(MAKE) $(ENVPARMS) lintit
	cd $(SGSBASE)/strip/m32; $(MAKE) $(ENVPARMS) lintit
	cd $(SGSBASE)/tsort/m32; $(MAKE) $(ENVPARMS) lintit
#
toollintit:
	cd $(SGSBASE)/yacc/m32; $(MAKE) $(ENVPARMS) $(CCPARMS) lintit 
	cd $(SGSBASE)/lex/m32; $(MAKE) $(ENVPARMS) $(CCPARMS) lintit 
#
liblintit:
	cd $(SGSBASE)/libelf/m32; $(MAKE) $(ENVPARMS) lintit 
	cd $(SGSBASE)/libld/m32; $(MAKE) $(ENVPARMS) lintit 
#
clean: toolclean libclean
	cd $(SGSBASE)/ar/m32; $(MAKE) $(ENVPARMS) $(CCPARMS) clean
	cd $(SGSBASE)/acomp/m32mau; $(MAKE) $(ENVPARMS) clean
	cd $(SGSBASE)/acomp/m32; $(MAKE) $(ENVPARMS) clean
	cd $(SGSBASE)/cpp/m32; $(MAKE) $(ENVPARMS) clean
	cd $(SGSBASE)/cmd/m32; $(MAKE) $(ENVPARMS) clean
	cd $(SGSBASE)/newoptim/m32; $(MAKE) $(ENVPARMS) clean
	cd $(SGSBASE)/as/m32; $(MAKE) $(ENVPARMS) clean
	cd $(SGSBASE)/ld/m32; $(MAKE) $(ENVPARMS) clean
	cd $(SGSBASE)/dis/m32; $(MAKE) $(ENVPARMS) clean
	cd $(SGSBASE)/dump/m32; $(MAKE) $(ENVPARMS) clean
	if [ X"$(NATIVE)" = Xyes ] ; \
	then \
		if [ "$(MNAME)" = "u3b2" ] ; \
		then \
			cd $(SGSBASE)/hdwr_tst/m32; $(MAKE) $(ENVPARMS) clean ; \
		fi \
	fi
	cd $(SGSBASE)/alint/m32; $(MAKE) $(ENVPARMS) clean
	cd $(SGSBASE)/cof2elf/m32; $(MAKE) $(ENVPARMS) clean
	cd $(SGSBASE)/lorder/m32; $(MAKE) $(ENVPARMS) clean
	cd $(SGSBASE)/lprof/m32; $(MAKE) $(ENVPARMS) clean
	cd $(SGSBASE)/m4/m32; $(MAKE) $(ENVPARMS) clean 
	cd $(SGSBASE)/mcs/m32; $(MAKE) $(ENVPARMS) clean 
	cd $(SGSBASE)/nm/m32; $(MAKE) $(ENVPARMS) clean
	cd $(SGSBASE)/size/m32; $(MAKE) $(ENVPARMS) clean
	cd $(SGSBASE)/strip/m32; $(MAKE) $(ENVPARMS) clean
	cd $(SGSBASE)/tsort/m32; $(MAKE) $(ENVPARMS) clean
#
toolclean:
	cd $(SGSBASE)/yacc/m32; $(MAKE) $(ENVPARMS) $(CCPARMS) clean 
	cd $(SGSBASE)/lex/m32; $(MAKE) $(ENVPARMS) $(CCPARMS) clean 
#
libclean:
	cd $(SGSBASE)/libelf/m32; $(MAKE) $(ENVPARMS) clean
	cd $(SGSBASE)/libld/m32; $(MAKE) $(ENVPARMS) clean
#
clobber: toolclobber libclobber
	cd $(SGSBASE)/ar/m32; $(MAKE) $(ENVPARMS) $(CCPARMS) clobber
	cd $(SGSBASE)/acomp/m32mau; $(MAKE) $(ENVPARMS) clobber
	cd $(SGSBASE)/acomp/m32; $(MAKE) $(ENVPARMS) clobber
	cd $(SGSBASE)/cpp/m32; $(MAKE) $(ENVPARMS) clobber
	cd $(SGSBASE)/cmd/m32; $(MAKE) $(ENVPARMS) clobber
	cd $(SGSBASE)/newoptim/m32; $(MAKE) $(ENVPARMS) clobber
	cd $(SGSBASE)/as/m32; $(MAKE) $(ENVPARMS) clobber
	cd $(SGSBASE)/ld/m32; $(MAKE) $(ENVPARMS) clobber
	cd $(SGSBASE)/dis/m32; $(MAKE) $(ENVPARMS) clobber
	cd $(SGSBASE)/dump/m32; $(MAKE) $(ENVPARMS) clobber
	if [ X"$(NATIVE)" = Xyes ] ; \
	then \
		if [ "$(MNAME)" = "u3b2" ] ; \
		then \
			cd $(SGSBASE)/hdwr_tst/m32; $(MAKE) $(ENVPARMS) clobber ; \
		fi \
	fi
	cd $(SGSBASE)/alint/m32; $(MAKE) $(ENVPARMS) clobber
	cd $(SGSBASE)/cof2elf/m32; $(MAKE) $(ENVPARMS) clobber
	cd $(SGSBASE)/lorder/m32; $(MAKE) $(ENVPARMS) clobber
	cd $(SGSBASE)/lprof/m32; $(MAKE) $(ENVPARMS) clobber
	cd $(SGSBASE)/m4/m32; $(MAKE) $(ENVPARMS) clobber 
	cd $(SGSBASE)/mcs/m32; $(MAKE) $(ENVPARMS) clobber 
	cd $(SGSBASE)/nm/m32; $(MAKE) $(ENVPARMS) clobber
	cd $(SGSBASE)/size/m32; $(MAKE) $(ENVPARMS) clobber
	cd $(SGSBASE)/strip/m32; $(MAKE) $(ENVPARMS) clobber
	cd $(SGSBASE)/tsort/m32; $(MAKE) $(ENVPARMS) clobber
#
toolclobber:
	cd $(SGSBASE)/yacc/m32; $(MAKE) $(ENVPARMS) $(CCPARMS) clobber 
	cd $(SGSBASE)/lex/m32; $(MAKE) $(ENVPARMS) $(CCPARMS) clobber 
#
libclobber:
	cd $(SGSBASE)/libelf/m32; $(MAKE) $(ENVPARMS) clobber
	cd $(SGSBASE)/libld/m32; $(MAKE) $(ENVPARMS) clobber
