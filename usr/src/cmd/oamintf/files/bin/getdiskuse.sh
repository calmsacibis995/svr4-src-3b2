#!/sbin/sh
#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)filemgmt:bin/getdiskuse.sh	1.3.1.1"
#	Copyright (c) 1984 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)filemgmt:bin/getdiskuse.sh	1.3.1.1"

echo "" > /tmp/disk.use

fslist=`/etc/mount  |  cut -d' ' -f1  |  sort`
echo "
   FILE SYSTEM USAGE AS OF" `date '+%m/%d/%y %T'` > /tmp/disk.use
echo " 

   File			Free	Total	Percent
   System		Blocks	Blocks	Full
   ------		------	------	-------" >> /tmp/disk.use
for fs in ${fslist}
{
	eval `df -t ${fs}  |
		sed '	1s/.*): *\([0-9]*\) .*/free=\1/
			2s/[^0-9]*\([0-9]*\) .*/total=\1/'`
	if [ "${total}" -gt 0 ]
	then
		percent=`expr \( ${total} - ${free} \) \* 100 / ${total}`%
	else
		percent=
	fi
	echo "   ${fs}		${free}	${total}	${percent}" >> /tmp/disk.use
}
