#!/sbin/sh
#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)oamintf:devices/remove/rmdisk.sh	1.4"

################################################################################
#	Module Name: rmdisk
#
#	Arguments:  $1 - device alias
################################################################################

bdevice=`/usr/bin/devattr $1 bdevice` > /dev/null
dskname=`expr "${bdevice}" : '.*/\([^/]*\).$'`
bdskpath=`expr "${bdevice}" : '\(.*\).'`

#  remove disk and partition information from device tables

for dpart in `/usr/bin/getdev type=dpart`
do
	devattr $1 dpartlist | grep ${dpart} > /dev/null
	if [ $? -eq 0 ]
	then
		/usr/bin/putdev -d ${dpart} > /dev/null
		/usr/bin/putdgrp -d dpart ${dpart} > /dev/null
	fi
done

/usr/bin/putdev -d $1 > /dev/null
/usr/bin/putdgrp -d disk $1 > /dev/null

# remove vfstab entries

grep -v "^${bdskpath}" /etc/vfstab > /tmp/$$.vfstab
mv /tmp/$$.vfstab /etc/vfstab

# remove device nodes

rm -f /dev/dsk/${dskname}? /dev/SA/$1 /dev/rdsk/${dskname}? /dev/rSA/$1
exit 0
