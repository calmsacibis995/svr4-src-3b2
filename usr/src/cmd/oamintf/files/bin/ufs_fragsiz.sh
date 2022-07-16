#!/sbin/sh
#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)filemgmt:bin/ufs_fragsiz.sh	1.1"
BLOCKSIZE=$1
if [ $BLOCKSIZE -eq 4096 ]
then
	echo 512
	echo 1024
	echo 2048
	echo 4096
else
	echo 1024
	echo 2048
	echo 4096
	echo 8192
fi
exit 0
	
