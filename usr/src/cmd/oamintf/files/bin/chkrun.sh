#!/sbin/sh
#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)filemgmt:bin/chkrun.sh	1.2"

echo "   Please Wait, interactive file system check is in progress."
flags="-qq -k$$"	# flags for checkyn to implement [q] (quit)

trap 'exit 1' 1 2 15

pid=$$

arg1=${1}
fstype=${2}
bdrv=`devattr ${arg1} bdevice`
cdrv=`devattr ${arg1} cdevice`
pdrv=`devattr ${arg1} pathname`

if  [ $bdrv ] 
then ddrive=$bdrv
else if  [ $cdrv ] 
	then ddrive=$cdrv
	else if  [ $pdrv ] 
		then ddrive=$pdrv
		else 	
			echo "Error - ${arg1} does not have a device pathname" 
			exit 1
     	     fi
     fi
fi

ndrive="${arg1} drive"
l=`/usr/sbin/labelit -F ${fstype}  ${ddrive} 2>/dev/null`

eval `/usr/lbin/labelfsname "${l}"`

#Select:
#	interactive repair

#	Interrupting an fsck can be dangerous and cause strange messages,
#	therefore, they are ignored.
trap '' 2

/sbin/fsck -F ${fstype} -D ${ddrive} 
exit=$?
trap 'exit 0' 2

if [ ${exit} -ne 0 ]
then
	echo "
   File system check did not finish successfully.
   Either the removable medium is damaged beyond this procedure's ability
   to repair it or there is some other problem.  Please call your service
   representative if you think there is a machine problem. " 
else
	trap "	trap '' 1 2
#	/usr/lbin/diskumount -n '${ndrive}' ${ddrive}" 0 
#	/sbin/mount -F ${fstype} ${ddrive} ${3} -r
#	if [ ! -d ${3}/lost+found ]
#	then
#		echo "   There is no lost+found directory on file system '${3}'" 
#		exit 0
#	fi
if [  -d ${3}/lost+found ]
then
	cd ${3}/lost+found
	if [ -n "`ls`" ]
	then
		echo "
   There are some files in the ${3}/lost+found directory.
   These are files that belong in the file system somewhere, but whose
   names have been lost, but whose contents is still intact.
   You should mount this file system, look at each file and if you
   recognize it, move it to the proper directory and name." 
	fi
	cd /
fi
fi
