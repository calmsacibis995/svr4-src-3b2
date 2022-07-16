#!/sbin/sh
#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)filemgmt:bin/numblocks.sh	1.2"
DEVICE=$1
#get the block device
if [ ! -b $DEVICE ]
then
	BDEVICE=`devattr "$DEVICE" bdevice  2>/dev/null`
else
	BDEVICE="$DEVICE"
fi
# if device is diskette use defaults else read size from prtvtoc o/p.
majmin1=`ls -l /dev/diskette | /usr/bin/tr -s '\040' '\011' |/usr/bin/cut -d"	" -f5,6`
majmin2=`ls -l "$BDEVICE" | /usr/bin/tr -s '\040' '\011' |/usr/bin/cut -d"	" -f5,6`
#ls -l "$BDEVICE" > /tmp/device
#majmin1=`cut -c35-40 /tmp/floppy`
#majmin2=`cut -c35-40 /tmp/device`
if [ "$majmin1" = "$majmin2" ]
then
	blocksize=1422
	echo "$blocksize"
	exit 0
else
	CDEVICE=`devattr "$BDEVICE" cdevice 2>/dev/null`
	prtvtoc "$CDEVICE" > /tmp/vtocout
	echo "$BDEVICE" > /tmp/bdevice
	slice=`cut -f4 -d"/" /tmp/bdevice |cut -f2 -d"s"`
	while read partition tag flags first total last
	do
		case "$partition" in
		'*'* | '')
			continue
		esac
		if [ "$partition" -eq "$slice" ] 
		then
			blocksize="$total"
			echo "$blocksize"
			exit 0
		fi
	done < /tmp/vtocout
fi
exit 0
