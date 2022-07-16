#!/sbin/sh
#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)initpkg:rc6.sh	1.3.5.1"

if u3b2
then
echo "
#	"Run Commands" for init state 6
#	Do auto-configuration now if necessary.
#

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
/usr/sbin/killall  9

/sbin/ckconfig /stand/unix /stand/system /dgn/edt_data

if [ \$? = 0 ]
then
	echo \"A new unix is being built\"

	/sbin/buildsys

	if [ \$? != 0 ]
	then
		echo \"auto-configuration failed, consult a System Administrator\"
		echo \"executing /sbin/sh, to continue shutdown sequence exit shell\"
		/sbin/sh
	fi
fi

/usr/sbin/nodgmon
/sbin/umountall
echo '
The system is down.'

# check if user wants machine restarted
case \"\$1\" in
        reboot)	echo \"\\nThe system is being restarted.\"
		/sbin/uadmin 2 1
	  	;;
esac
" > rc6
fi
