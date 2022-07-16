#!/sbin/sh
#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:devices/valdisk.sh	1.2"

################################################################################
#	Module Name: valdisk.sh
#	
#	Inputs:
#		$1 - group
#		$2 - device
#	
#	Description: Verify that a valid disk was entered.
################################################################################
if [ $1 ] 
then
	list=`listdgrp $1`

	for x in `getdev type=disk $list`
	do
		if [ "x$2" = "x$x" ]
		then
			break
		fi
	done
	if [ "x$2" != "x$x" ]
	then
		exit 1
	fi
fi

if [ "`getdev type=disk $2`" = "$2" ]
then
	root=`getdev -a type=dpart mountpt=/`
	usr=`getdev -a type=dpart mountpt=/usr`

	IFS=" 	,"
	reserved=no
	for dpart in `devattr $2 dpartlist`
	do
		if [ $dpart = $usr ] || [ $dpart = $root ] 
		then
			reserved=yes
			break
		fi
		bdev=`devattr $dpart bdevice`
		/sbin/mount | grep ${bdev} > /dev/null
		if [ $? -eq 0 ]
		then
			reserved=yes
			break
		fi
	done
	[ "$reserved" = no ] && exit 0
fi

exit 1
