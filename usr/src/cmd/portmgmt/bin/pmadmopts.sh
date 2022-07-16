#!/sbin/sh
#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)portmgmt:bin/pmadmopts.sh	1.3"

# pmadmopts - form the pmadm command line

opts=""
if [ "$1" = DISABLED ]
then
	opts="$opts -fx"
fi

if [ "$2" = Yes ]
then
	opts="$opts -fu"
fi

if test -n "$3"
then
	opts="$opts -z $3"
fi

if test -n "$4"
then
	opts="$opts -y \"$4\""
fi

echo "$opts" \\ 
