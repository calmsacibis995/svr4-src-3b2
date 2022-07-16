#!/sbin/sh
#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:devices/erase/floperase.sh	1.2"

################################################################################
#	Module Name: floperase.sh
################################################################################

lumps=200	#	Number of blocks per erased segment of media.

a=`spclsize -b $1`

while [ $a -gt 0 ]
do
	echo ""  |  dd bs=${lumps}b conv=sync 2>/dev/null
	a=`expr $a - ${lumps}`
done  >$1
