#!/sbin/sh
#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)bkrs:intftools.d/valweeks.sh	1.5"
TABLE=$3
if [ "$1" = "demand" ]
then
	exit 0
elif [ "$1" = "$2" ]
then
	exit 0
else
	WEEKS=`echo "$1" | sed -e "s/  */,/g"`
	PERIOD=`getrpd $TABLE`
	validweeks "$WEEKS" $PERIOD
	exit $?
fi
