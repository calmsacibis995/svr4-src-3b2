#!/sbin/sh
#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)filemgmt:bin/checkfs.sh	1.3.1.1"

echo "" > /tmp/check.err
flags="-qq -k$$"	# flags for checkyn to implement [q] (quit)

trap 'exit 1' 1 2 15

pid=$$

bdrv=`devattr ${1} bdevice`
cdrv=`devattr ${1} cdevice`
pdrv=`devattr ${1} pathname`
if  [ $bdrv ] 
then ddrive=$bdrv
else if  [ $cdrv ] 
	then ddrive=$cdrv
	else if  [ $pdrv ] 
		then ddrive=$pdrv
		else 	
			echo "   Error - ${1} does not have a device pathname" >>/tmp/check.err
			exit 1
     	     fi
     fi
fi

ndrive="${1} drive"
checktype=${2}
fstype=${3}

#l=`/sbin/labelit -F ${fstype}  ${ddrive} 2>/dev/null`

#eval `/usr/lbin/labelfsname "${l}"`

#	The "check" looks for file system damage but does not attempt to
#	fix it.  The "interactive repair" asks you to decide what should be
#	done for each error.  The "automatic repair" makes standard repairs
#	when it finds errors;  it is generally safe, although there are some 
#	problems it cannot handle.  In those cases, you need a file system
#	repair expert to fix things.
#	     Most users will be satisfied using "automatic".  For particularly
#	sensitive or unreproducible data we recommend that you use "check"
#	first and then use either "interactive" or "automatic" repair.' \
#	"Disk '${label}', file system '/${fsname}'.
#Select:
#	check only 
#	interactive repair
#	automatic repair
#	Interrupting an fsck can be dangerous and cause strange messages,
#	therefore, they are ignored.
trap '' 2
case "${checktype}" in
"check only")
	/sbin/fsck -F ${fstype} -n ${ddrive} >>/tmp/check.err 2>>/tmp/check.err
	;;
"interactive fix")
	/sbin/fsck -F ${fstype} ${ddrive} >>/tmp/check.err 2>>/tmp/check.err
	;;
"automatic fix")
	/sbin/fsck -F ${fstype} -y ${ddrive} >>/tmp/check.err 2>>/tmp/check.err
	;;
* )
	echo "  Invalid checktype '${checktype}'" >>/tmp/check.err 2>>/tmp/check.err
	exit 1
esac
exit=$?

trap 'exit 9' 2

if [ ${exit} -ne 0 ]
then
	echo "
   File system check did not finish successfully.
   Either the removable medium is damaged beyond this procedure's ability
   to repair it or there is some other problem.  Please call your service
   representative if you think there is a machine problem.
   " >>/tmp/check.err
else
	trap "	trap '' 1 2
#	/usr/lbin/diskumount -n '${ndrive}' ${ddrive}" 0 >>/tmp/check.err 2>>/tmp/check.err
#	/sbin/mount -F ${fstype} ${ddrive} ${4} -r >>/tmp/check.err 2>>/tmp/check.err
#	if [ ! -d ${4}/lost+found ]
#	then
#		echo "   There is no lost+found directory on file system '${4}'" >>/tmp/check.err
#		exit 0
#	fi
if [ -d ${4}/lost+found ]
then
	cd ${4}/lost+found
	if [ -n "`ls`" ]
	then
		echo "
   There are some files in the ${4}/lost+found directory.
   These are files that belong in the file system somewhere, whose
   names have been lost, but whose contents is still intact.
   You should mount this file system, look at each file and if you
   recognize it, move it to the proper directory and name." >>/tmp/check.err
	fi
	cd /
fi
fi
