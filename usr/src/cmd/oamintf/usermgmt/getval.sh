#!/sbin/sh
#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:usermgmt/getval.sh	1.1"
# THis command gets defaulta values of MAXWEEKS and MINWEEKS for
# password when not defined in /etc/shadow. It can be modified for
# login also.

case $2 in
"")
	exit;;
esac
case $1 in
-x)  	

	VAL=`passwd -s $2 | sed -n -e "s/^[^ ]*[ ]*[^ ]*[ ]*[^ ]*[ ]*[^ ]*[ ]*\([^ ]*\).*/\1/p"`
	echo ${VAL:-`getdfl -x /etc/default/passwd`};;
-n) 
	VAL=`passwd -s $2 | sed -n -e "s/^[^ ]*[ ]*[^ ]*[ ]*[^ ]*[ ]*\([^ ]*\).*/\1/p" `
	echo ${VAL:-`getdfl -n /etc/default/passwd`};;
esac
