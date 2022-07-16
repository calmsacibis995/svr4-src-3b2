#!/sbin/sh
#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)lnttys:lnxts.sh	1.1"
# install links to /dev sub-directories

cd /dev/xt
for i in *
do
	rm -f /dev/xt$i
	ln /dev/xt/$i /dev/xt$i
done
