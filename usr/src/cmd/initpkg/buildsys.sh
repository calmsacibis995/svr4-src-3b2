#!/sbin/sh
#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)initpkg:buildsys.sh	1.2.3.2"

if u3b2
then	echo "
shutsys=\"no\"
while [ \$# -gt 0 ]
do
	case \$1 in
	-s )
		shutsys=\"yes\"
		;;
	* )
		echo \"Usage: \$0 [-s]\"
		exit 1
		;;
	esac
	shift
done

mountall /etc/boot_tab

sfile=\`/usr/bin/cat /etc/.sysfile\`
ofile=\"/stand/unix\"

echo \"\\n\$ofile is being created\\n\"

umask 022

touch /stand/system

/usr/sbin/cunix -f \$sfile -v -d -o \$ofile

rval=\$?

if [ \$rval = 0 -a \"\$shutsys\" = \"yes\" ]
then
	echo \"\\n\$ofile created, system will reboot automatically\"
	/usr/sbin/nodgmon
	/sbin/umountall
	/sbin/sync
	/sbin/sync
	/sbin/sync
	/sbin/sync
	/sbin/uadmin 2 1
fi
exit \$rval
" > buildsys
fi
