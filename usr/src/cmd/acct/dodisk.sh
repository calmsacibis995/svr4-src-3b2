#!/sbin/sh
#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)acct:dodisk.sh	1.13"
# 'perform disk accounting'
PATH=:/usr/lib/acct:/usr/bin:/usr/sbin
export PATH
if [ -f  /usr/bin/uts -a /usr/bin/uts ]
then
	format="dev mnt type comment"
else 
	format="special dev mnt fstype fsckpass automnt mntflags"
fi
_dir=/var/adm
_pickup=acct/nite
set -- `getopt o $*`
if [ $? -ne 0 ]
then
	echo "Usage: $0 [ -o ] [ filesystem ... ]"
	exit 1
fi
for i in $*; do
	case $i in
	-o)	SLOW=1; shift;;
	--)	shift; break;;
	esac
done

cd ${_dir}

if [ "$SLOW" = "" ]
then
	if [ $# -lt 1 ]
	then
		if [ -f  /usr/bin/uts -a /usr/bin/uts ]
		then
			DEVLIST=/etc/checklist
		else
			DEVLIST=/etc/vfstab
		fi
		while :
		do
			if read $format
		       	then
				if [ $fsckpass -ne 1 ]
				then
					continue
				fi
				if [ $fstype != s5 ]
				then
					continue
				fi
				if [ `expr $dev : '\(.\)'` = \# ]
		       		then
		               		continue
		        	fi
				if [ -f  /usr/bin/uts -a /usr/bin/uts ]
				then
					if [ $type = u370 ]
					then
						u370diskusg $dev > `basename $mnt`.dtmp &
					else
						echo diskusg $dev > `basename $mnt`.dtmp &
					fi
				else
					diskusg $dev > `basename $dev`.dtmp &
				fi
			else
				wait
				break
			fi
		done < $DEVLIST
		cat *.dtmp | diskusg -s > dtmp
		rm -f *.dtmp
	else
		diskusg $* > dtmp
	fi
else
	if [ $# -lt 1 ]
	then
		args="/"
	else
		args="$*"
	fi
	for i in $args; do
		if [ ! -d $i ]
		then
			echo "$0: $i is not a directory -- ignored"
		else
			dir="$i $dir"
		fi
	done
	if [ "$dir" = "" ]
	then
		echo "$0: No data"
		> dtmp
	else
		find $dir -print | acctdusg > dtmp
	fi
fi

sort +0n +1 dtmp | acctdisk > ${_pickup}/disktacct
chmod 644 ${_pickup}/disktacct
chown adm ${_pickup}/disktacct
