#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)autoconfig:map4.0/map4.0.mk	1.3"

DIR=$(ROOT)/config
INS = install

all:


install: all
	$(INS) -f $(DIR) -m 0444 -u bin -g bin  mapfile4.0
	
clobber:

