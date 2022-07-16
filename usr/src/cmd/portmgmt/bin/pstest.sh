#!/sbin/sh
#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)portmgmt:bin/pstest.sh	1.2"

# pstest - check to see if there is at least one service 
#	   to enable, disable or remove.
# Input:   $1 - type of operation, (i.e. enable, disable, or remove)

OK=0		# at least one item is found
NOTHING=1	# no item is found

case $1 in
	enable)
		psfile=`pmadm -L|cut -d: -f4|grep "x"| wc -l`;;
	disable)
		psfile=`pmadm -L|cut -d: -f4|grep -v "x"| wc -l`;;
	remove)
		psfile=`pmadm -L| wc -l`;;
	*)
		exit $NOTHING;;
esac

if [ $psfile = 0 ]
then
	exit $NOTHING
else
	exit $OK
fi
