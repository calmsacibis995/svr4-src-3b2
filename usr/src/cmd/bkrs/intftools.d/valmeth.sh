#!/sbin/sh
#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)bkrs:intftools.d/valmeth.sh	1.2"
# validate that a method is one of the existing ones
MDIR=`brfindtab method`
METHODS=`ls -m $MDIR | sed -e "s/,/ /g"`
for i in $METHODS
do
	if [ "$i" = "$1" ]
	then
		exit 0
	fi
done
exit 1

