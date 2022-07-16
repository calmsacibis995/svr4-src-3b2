#!/sbin/sh
#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)face:src/xx/unix.sh	1.2"
echo To return, type \`exit\` or control-d
echo You are in `pwd`
exec ${SHELL:-/bin/sh}
