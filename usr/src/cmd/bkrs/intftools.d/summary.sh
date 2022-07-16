#!/sbin/sh
#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)bkrs:intftools.d/summary.sh	1.4"

WEEKS="$2"
DAYS="$3"
OPTS="-t $1"
TFILE=/tmp/bkreg$$

if [ "$2" != "all" -o "$3" != "all" ]
then
	if [ "$2" = "all" ]
	then
		WEEKS="1-52"
	fi

	if [ "$3" = "all" ]
	then
		DAYS="0-6"
	fi

	if [ "$2" = "demand" ]
	then
		SELECT=$WEEKS
	else
		SELECT=$WEEKS:$DAYS
	fi
	OPTS="$OPTS -c \"$SELECT\""
fi

eval bkreg $OPTS >$TFILE 2>&1
RC=$?
echo $TFILE
exit $RC
