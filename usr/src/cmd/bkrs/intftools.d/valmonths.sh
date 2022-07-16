#!/sbin/sh
#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)bkrs:intftools.d/valmonths.sh	1.1"
if [ "$1" = "all" ]
then
	exit 0
else
	MONS=`echo "$1" | sed -e "s/  */,/g"`
	validmons $MONS
	exit $?
fi

