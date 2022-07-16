#!/sbin/sh
#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:usermgmt/modgrp.sh	1.3"

################################################################################
#	Command Name: modgrp
#
#	Description: This scripts does 3 things: 1) modifies group information
#		     2) changes primary group for specified logins 3) adds
#		     supplementary group status to specified logins.
#
# 	Inputs:		$1 - Group name
#			$2 - New Group name
# 			$3 - group ID
# 			$4 - primary group
# 			$5 - supplementary groups
################################################################################

# There are two pieces of information about the group entry that can
#   change: name and ID.  Since both have been validated and tested for
#   uniqueness prior to calling this command, we can use the override
#   (-o) option when changing the group info.  However, we do have to test
#   if the name has changed or not.
if [ $2 = $1 ]
then
	groupmod -g $3 -o $1
else
	groupmod -g $3 -o -n $2 $1
fi

# change primary group for specified logins
for x in `echo $4 | sed 's/,/ /g'`
do
	usermod -g "$1" "$x" || exit 1
done

# change supplementary group members
if [ $5 ]
then
	addgrpmem -g $1 `echo $5 | sed 's/,/ /g'` > /dev/null
fi
