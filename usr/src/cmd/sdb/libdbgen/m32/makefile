#	Copyright (c) 1988 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)sdb:libdbgen/m32/makefile	1.5"

TARGET = $(PRODLIB)/libdbgen.a

include ../../util/std.make

CSOURCES = \
	$(COMMON)/num.c

CCSOURCES = \
	$(COMMON)/Avltree.C \
	$(COMMON)/Link.C \
	$(COMMON)/Vector.C \
	$(COMMON)/Wordstack.C \
	$(COMMON)/str.C

include DEPEND

include ../../util/lib.make
