#	Copyright (c) 1988 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)sdb:libsymbol/m32/makefile	1.7"

TARGET = $(PRODLIB)/libsymbol.a

include ../../util/std.make

CCSOURCES = \
	$(COMMON)/AddrList.C \
	$(COMMON)/Bdi.C \
	$(COMMON)/Bdibuild.C \
	$(COMMON)/Cache.C \
	$(COMMON)/Coff.C \
	$(COMMON)/Coffbuild.C \
	$(COMMON)/Elfbuild.C \
	$(COMMON)/Evaluator.C \
	$(COMMON)/Fetalline.C \
	$(COMMON)/Fetalrec.C \
	$(COMMON)/Locdesc.C \
	$(COMMON)/NameList.C \
	$(COMMON)/Object.C \
	$(COMMON)/Reflist.C \
	$(COMMON)/SectHdr.C \
	$(COMMON)/Source.C \
	$(COMMON)/Symbol.C \
	$(COMMON)/Symtab.C \
	$(COMMON)/Symtable.C \
	$(COMMON)/TYPE.C \
	$(COMMON)/Tagcheck.C \
	$(COMMON)/builder.C \
	$(COMMON)/dbtools.C \
	$(COMMON)/dbtools1.C

include DEPEND

include ../../util/lib.make
