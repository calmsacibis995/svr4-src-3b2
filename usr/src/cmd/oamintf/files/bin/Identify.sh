#!/sbin/sh
#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)filemgmt:bin/Identify.sh	1.1.1.2"
FSYS=`devattr  ${1} bdevice 2> /dev/null`
if [ "t$FSYS" = "t" ]
then
	FSYS=$1
fi
echo "Possible type(s):\n\n" > /tmp/fstype
/usr/sbin/fstyp ${FSYS} >> /tmp/fstype.out
case $? in
	0) cat /tmp/fstype.out >> /tmp/fstype;;
	1) echo "cannot identify file system type" >> /tmp/fstype;;
	2) cat /tmp/fstype.out >> /tmp/fstype;
	  echo "\nWarning: more than one fstype identified.\nIf you are going to check this file system, you should\nselect the 'check only' option." >>/tmp/fstype;;
esac
rm -f /tmp/fstype.out
echo "" >> /tmp/fstype
