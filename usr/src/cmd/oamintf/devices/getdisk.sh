#!/sbin/sh
#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:devices/getdisk.sh	1.1"
################################################################################
# 	Name: getdisk
#		
#	Desc: Get disk choices for partition or remove that doesn't have
#	      '/' or '/usr' on them.
#	
#	Arguments: $1 - device group
################################################################################
if [ "$1" ]
then
	
	list=`listdgrp $1`
	for disk in `getdev type=disk $list`
	do
		root=`getdev -a type=dpart mountpt=/`
		usr=`getdev -a type=dpart mountpt=/usr`

		IFS=" 	,"
		reserved=no
		for dpart in `devattr $disk dpartlist`
		do
			if [ $dpart = $usr ] || [ $dpart = $root ] 
			then
				reserved=yes
				break
			fi
		done
		if [ "$reserved" = no ]
		then
			echo "$disk\072\c"; devattr $disk desc
		fi
			
	done
else
	for disk in `getdev type=disk`
	do
		root=`getdev -a type=dpart mountpt=/`
		usr=`getdev -a type=dpart mountpt=/usr`

		IFS=" 	,"
		reserved=no
		for dpart in `devattr $disk dpartlist`
		do
			if [ $dpart = $usr ] || [ $dpart = $root ] 
			then
				reserved=yes
				break
			fi
		done
		if [ "$reserved" = no ]
		then
			echo "$disk\072\c"; devattr $disk desc
		fi
	done
fi
