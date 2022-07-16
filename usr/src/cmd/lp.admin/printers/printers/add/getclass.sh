#!/sbin/sh
#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)lp.admin:printers/printers/add/getclass.sh	1.2"
# getclass.sh : Get only the classes linked to the printer
# to use for default values.

if [ "$lp_default" = "none" ]
then
	echo none;
	exit;
fi
if grep "^$lp_default$" /etc/lp/classes/* > /tmp/class.$$;
then
	cat /tmp/class.$$ | sed 's/\/etc\/lp\/classes\///gp' |
#        cut -f1 -d":" | tr "[\012*]" "[\040*]";
        cut -f1 -d":" | tail -1f;
else
	echo none;
fi;
rm -f /tmp/class.$$
