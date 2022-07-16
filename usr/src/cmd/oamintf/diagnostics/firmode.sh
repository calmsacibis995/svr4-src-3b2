#!/sbin/sh
#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:diagnostics/firmode.sh	1.3"

################################################################################
#	Module Name: firmmode.sh
#
#	Description: cd to / and enter firmware mode.
#
#	Args:
#		$1 - number of seconds before shutdown
#		$2 - system state
################################################################################

cd /
shutdown -y -g"$1" -i$2
