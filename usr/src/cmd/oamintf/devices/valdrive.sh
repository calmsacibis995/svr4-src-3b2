#!/sbin/sh
#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:devices/valdrive.sh	1.4"

################################################################################
#	Module Name: valdrive.sh
#	
#	Inputs:
#		$1 - group
#		$2 - type
#		$3 - device
#	
#	Description: Verify a valid device was entered.
################################################################################
if [ $1 ] 
then
	list=`listdgrp $1`

	for x in `getdev type=$2 $list`
	do
		if [ "x$3" = "x$x" ]
		then
			exit 0
		fi
	done
	exit 1

elif [ "`getdev type=$2 $3`" = "$3" ]
then
	exit 0

else
	exit 1
fi
