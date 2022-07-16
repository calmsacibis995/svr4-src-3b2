#!/sbin/sh
#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)netadm:iu.ap	1.3"

if u3b2
then echo "# /dev/console and /dev/contty autopush setup
#
# major	minor	lastminor	modules

    0	  -1	    0		ldterm
" >iu.ap
fi
