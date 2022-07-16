#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)devmgmt:devmgmt.mk	1.7"

SUBMAKES=devattr getdev getdgrp listdgrp devreserv devfree data putdev putdgrp getvol

foo		: all

.DEFAULT	:	
		for submk in $(SUBMAKES) ; \
		do \
		    cd $$submk ; \
		    $(MAKE) -f $$submk.mk $@ ; \
		    cd .. ; \
		done
