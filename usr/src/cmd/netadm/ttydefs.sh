#!/sbin/sh
#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)netadm:ttydefs	1.2"

if u3b2
then echo "# VERSION=1
38400:38400 hupcl erase ^h:38400 sane ixany tab3 hupcl erase ^h::19200
19200:19200 hupcl erase ^h:19200 sane ixany tab3 hupcl erase ^h::9600
9600:9600 hupcl erase ^h:9600 sane ixany tab3 hupcl erase ^h::4800
4800:4800 hupcl erase ^h:4800 sane ixany tab3 hupcl erase ^h::2400
2400:2400 hupcl erase ^h:2400 sane ixany tab3 hupcl erase ^h::1200
1200:1200 hupcl erase ^h:1200 sane ixany tab3 hupcl erase ^h::300
300:300 hupcl erase ^h:300 sane ixany tab3 hupcl erase ^h::19200

auto:hupcl erase ^h:sane ixany tab3 hupcl erase ^h:A:9600

console:9600 hupcl opost onlcr erase ^h:9600 sane ixany tab3 erase ^h::console1
console1:1200 hupcl opost onlcr erase ^h:1200 sane ixany tab3 erase ^h::console2
console2:300 hupcl opost onlcr erase ^h:300 sane ixany tab3 erase ^h::console3
console3:2400 hupcl opost onlcr erase ^h:2400 sane ixany tab3 erase ^h::console4
console4:4800 hupcl opost onlcr erase ^h:4800 sane ixany tab3 erase ^h::console5
console5:19200 hupcl opost onlcr erase ^h:19200 sane ixany tab3 erase ^h::console

contty:9600 hupcl opost onlcr erase ^h:9600 sane ixany tab3 erase ^h::contty1
contty1:1200 hupcl opost onlcr erase ^h:1200 sane ixany tab3 erase ^h::contty2
contty2:300 hupcl opost onlcr erase ^h:300 sane ixany tab3 erase ^h::contty3
contty3:2400 hupcl opost onlcr erase ^h:2400 sane ixany tab3 erase ^h::contty4
contty4:4800 hupcl opost onlcr erase ^h:4800 sane ixany tab3 erase ^h::contty5
contty5:19200 hupcl opost onlcr erase ^h:19200 sane ixany tab3 erase ^h::contty

pty:9600 hupcl opost onlcr erase ^h:9600 sane ixany tab3 erase ^h::pty

4800H:4800 erase ^h:4800 sane ixany tab3 hupcl erase ^h::9600H
9600H:9600 erase ^h:9600 sane ixany tab3 hupcl erase ^h::19200H
19200H:19200 erase ^h:19200 sane ixany tab3 hupcl erase ^h::38400H
38400H:38400 erase ^h:38400 sane ixany tab3 hupcl erase ^h::2400H
2400H:2400 erase ^h:2400 sane ixany tab3 hupcl erase ^h::1200H
1200H:1200 erase ^h:1200 sane ixany tab3 hupcl erase ^h::300H
300H:300 erase ^h:300 sane ixany tab3 hupcl erase ^h::4800H

conttyH:9600 opost onlcr erase ^h:9600 hupcl sane ixany tab3 erase ^h::contty1H
contty1H:1200 opost onlcr erase ^h:1200 hupcl sane ixany tab3 erase ^h::contty2H
contty2H:300 opost onlcr erase ^h:300 hupcl sane ixany tab3 erase ^h::contty3H
contty3H:2400 opost onlcr erase ^h:2400 hupcl sane ixany tab3 erase ^h::contty4H
contty4H:4800 opost onlcr erase ^h:4800 hupcl sane ixany tab3 erase ^h::contty5H
contty5H:19200 opost onlcr erase ^h:19200 hupcl sane ixany tab3 erase ^h::conttyH
" >ttydefs
fi
