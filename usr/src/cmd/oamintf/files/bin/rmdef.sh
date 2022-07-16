#!/sbin/sh
#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Copyright (c) 1988 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)filemgmt:bin/rmdef.sh	1.3.1.1"
DEV=$1
FS=$2
> /tmp/vfstab
if [ ! -b $DEV ]
then
	BDEVICE=`devattr "$DEV"  bdevice 2>/dev/null`
else
	BDEVICE="$DEV"
fi
while read bdev rdev mountp fstype fsckpass automnt mntopts
do
	if test "$BDEVICE" != "$bdev" -o "$FS" != "$mountp"
	then
		echo $bdev $rdev $mountp $fstype $fsckpass $automnt $mntopts | awk '{printf("%-17s %-17s %-6s %-6s %-8s %-7s %-8s\n", $1, $2, $3, $4, $5, $6, $7)}' >>/tmp/vfstab
	fi
done < /etc/vfstab
cp /tmp/vfstab /etc/vfstab
