#!/sbin/sh
#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)filemgmt:bin/existsfs.sh	1.1"
DEVICE=$1
FSTYPE=$2
if [  ! -b $DEVICE ]
then
	DEVICE=`devattr $DEVICE bdevice 2> /dev/null`
fi
if fsck -F $FSTYPE -m $DEVICE 2> /dev/null
then
	echo "true"
	exit 0
else
	echo "false"
	exit 1
fi
