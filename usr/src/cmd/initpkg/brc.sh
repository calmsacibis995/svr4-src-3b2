#!/sbin/sh
#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)initpkg:./brc.sh	1.7.3.1"

if u3b2
then echo "if [ -f /etc/dfs/sharetab ]
then
	/usr/bin/mv /etc/dfs/sharetab /etc/dfs/osharetab
fi
if [ ! -d /etc/dfs ]
then /usr/bin/mkdir /etc/dfs
fi
>/etc/dfs/sharetab
# chmod 644 /etc/dfs/sharetab
" >>brc
fi
