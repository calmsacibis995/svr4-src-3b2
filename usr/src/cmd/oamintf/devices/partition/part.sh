#!/sbin/sh
#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:devices/partition/part.sh	1.5"

################################################################################
#	Module Name: part.sh
################################################################################

ALIAS=$1	# The alias for device found in device.tab
CDEVICE=$2	# The character device found in device.tab
DFILE=$3	# The data file created in Form.part
EFILE=$4	# The error file created in Form.part

# Check to see if the disk is already partitioned
prtvtoc $ALIAS $CDEVICE > /dev/null 2> /dev/null

# If the disk is already partitioned then modify device.tab
if [ $? = 0 ]
then
	# Remove partitions
	IFS="	 ,"
	for dpart in `devattr $ALIAS dpartlist`
	do
		putdev -d  $dpart
	done
fi

/sbin/fmthard -s $DFILE $CDEVICE > /dev/null 2>$ERR

