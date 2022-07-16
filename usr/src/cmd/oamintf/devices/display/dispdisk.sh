#!/sbin/sh
#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:devices/display/dispdisk.sh	1.3"

################################################################################
#	Module Name: disp.disk
################################################################################

ddrive=$1 # $1 = disk drive selected

#   The following commands arre from the old sysadm shell script display.
#   I'm not sure if I can do this in a better way.

ndrive=`/usr/lbin/drivename ${ddrive}` 			# ndirve = disk1 drive
dskname=`/usr/lbin/samedev ${ddrive} /dev/rdsk/c*d*s6`	
dskname=`basename ${dskname}`				# dskname = c?d?s6
havet=`expr ${dskname} : '.*\(t\).*'`

if [ -n "${havet}" ]
then

  eval `echo ${dskname} | sed -e 's:^c\([0-9]*\)t\([0-9]*\)d\([0-9]*\).*$:slot=\1 tc=\2 drv=\3:'`

  echo "Displaying ${ndrive} partitioning (hardware slot ${slot},"
  echo "target controller ${tc}, drive ${drv}):\n"

else

  eval `echo ${dskname} | sed -e 's:^c\([0-9]*\)d\([0-9]*\).*$:slot=\1 drv=\2:'`
  slot=`expr ${slot} - 1`
  echo "Displaying ${ndrive} partitioning (hardware slot ${slot} drive ${drv}):\n"
fi

/usr/sbin/prtvtoc ${ddrive}	

exit 0
