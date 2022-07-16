#!/sbin/sh
#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)initpkg:./rc0.sh	1.15.4.1"

if u3b2
then echo "
#	\"Run Commands\" for init state 0
#	Leaves the system in a state where it is safe to turn off the power
#	or go to firmware.
#
#	Takes an optional argument of either "off" or "firmware"
#	to specify if this is being run for "init 0" or "init 5."
#
#	In SVR4.0, inittab has been changed to no longer do the
#	uadmin to shutdown or enter firmware.  Instead, this script
#	is responsible.  By using an optional argument,
#	compatibility is maintained while still providing the needed
#	functionality to perform the uadmin call.

echo 'The system is coming down.  Please wait.'

# make sure /usr is mounted before proceeding since init scripts
# and this shell depend on things on /usr file system
/sbin/mount /usr > /dev/null 2>&1

#	The following segment is for historical purposes.
#	There should be nothing in /etc/shutdown.d.
if [ -d /etc/shutdown.d ]
then
	for f in /etc/shutdown.d/*
	{
		if [ -s \$f ]
		then
			/sbin/sh \${f}
		fi
	}
fi
#	End of historical section

if [ -d /etc/rc0.d ]
then
	for f in /etc/rc0.d/K*
	{
		if [ -s \${f} ]
		then
			/sbin/sh \${f} stop
		fi
	}

#	system cleanup functions ONLY (things that end fast!)	

	for f in /etc/rc0.d/S*
	{
		if [ -s \${f} ]
		then
			/sbin/sh \${f} start
		fi
	}
fi

trap \"\" 15
/usr/sbin/killall
/usr/bin/sleep 10
/sbin/umountall
echo '
The system is down.'

# check if user wants machine brought down or firmware mode
case \"\$1\" in
	off)	/sbin/uadmin 2 0
		;;

	firm*)	/sbin/uadmin 2 2
		;;
esac
" >rc0
fi
