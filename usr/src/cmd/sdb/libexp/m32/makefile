#	Copyright (c) 1988 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)sdb:libexp/m32/makefile	1.6"

TARGET = $(PRODLIB)/libexp.a

include ../../util/std.make

CSOURCES = \
	fetch.c

CCSOURCES = \
	Call.C \
	Call2.C \
	$(COMMON)/Expr.C \
	$(COMMON)/Label.C \
	$(COMMON)/Place.C \
	$(COMMON)/Rvalue.C \
	$(COMMON)/SDBinfo.C \
	$(COMMON)/format.C

include DEPEND

include ../../util/lib.make
