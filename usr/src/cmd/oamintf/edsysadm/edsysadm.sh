#!/sbin/sh
#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:edsysadm/edsysadm.sh	1.6"

################################################################################
#	Module Name: edsysadm
#	Date: July 87
################################################################################

if [ $# -ne 0 ]
then
	echo "	UX edsysadm: invalid syntax."
	echo "	Usage: edsysadm	(NO options or arguments)"
	exit
fi
	
OAMBASE=/usr/sadm/sysadm
INTFBASE=$OAMBASE/menu
EDSYSADM=$OAMBASE/edmenu
TESTBASE=/tmp
OBJ_DIR=/usr/sadm/sysadm/edmenu
PATH=/sbin:$OAMBASE/edbin:/usr/bin:/usr/sbin:/etc:/usr/sadm/bin:/usr/sadm/install/bin

export INTFBASE OAMBASE EDSYSADM TESTBASE PATH OBJ_DIR

# Execute fmli with the initialization file and
# top level edsysadm menu

fmli -i $INTFBASE/oam.init -c $INTFBASE/oam.cmd $EDSYSADM/Menu.pkg
