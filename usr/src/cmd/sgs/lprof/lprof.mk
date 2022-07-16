#	Copyright (c) 1988 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)lxprof:lxprof.mk	1.10.1.9"

LPROFBASE	= `pwd | sed 's/lprof.*/lprof/'`

SGSBASE		= $(LPROFBASE)/..
CMDBASE		= $(LPROFBASE)/../..

include lprofinc.mk

all:  basicblk cmds 

basicblk:
	cd bblk; $(MAKE) -f bblk.mk $(ENVPARMS)

cmds:
	if test "$(NATIVE)" = "yes"; then \
		cd cmd; $(MAKE) -f cmd.mk $(ENVPARMS); \
	fi

install: all
	$(MAKE) target -f lprof.mk LPTARGET=install $(ENVPARMS)

lintit:
	$(MAKE) target -f lprof.mk LPTARGET=lintit $(ENVPARMS)

clean:
	$(MAKE) target -f lprof.mk LPTARGET=clean $(ENVPARMS)

clobber: clean
	$(MAKE) target -f lprof.mk LPTARGET=clobber $(ENVPARMS)

target:
	if test "$(NATIVE)" = "yes"; then \
	   cd bblk; $(MAKE) -f bblk.mk $(LPTARGET) $(ENVPARMS); cd ..; \
	   cd cmd; $(MAKE) -f cmd.mk $(LPTARGET) $(ENVPARMS); cd ..; \
	   cd libprof; $(MAKE) -f libprof.mk $(LPTARGET) $(ENVPARMS); cd ..; \
	else \
	   cd bblk; $(MAKE) -f bblk.mk $(LPTARGET) $(ENVPARMS); cd ..; \
	fi
