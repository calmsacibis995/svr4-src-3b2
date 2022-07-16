#!/sbin/sh
#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:machinemgmt/chgstate.sh	1.2"

################################################################################
#	Module Name: chgstate.sh
#	Inputs:
#		$1 - grace period
#		$2 - init state
################################################################################

cd /
shutdown -y -g"$1" -i$2
