#!/sbin/sh
#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:syssetup/datechk.sh	1.1"
################################################################################
#	Module Name: datechk
#	Calling Sequence: This script is invoked through the Form.set
#			  form in "syssetup:datetime:set" menu.
#	Functional Description:	
#	Inputs: $1 is MM, $2 is Day and $3 is Year (ccxx).
#	Outputs: Exit code should always be 0.
#	Functions Called: 
###############################################################################

if test $# -ne 3 
then
	exit 1
fi

# assign month
case "${1}" in
	'Jan' | 'January')	mm="01";;
	'Feb' | 'February')	mm="02";;
	'Mar' | 'March')	mm="03";;
	'Apr' | 'April')	mm="04";;
	'May' | 'May')		mm="05";;
	'Jun' | 'June')		mm="06";;
	'Jul' | 'July')		mm="07";;
	'Aug' | 'August')	mm="08";;
	'Sep' | 'September')	mm="09";;
	'Oct' | 'October')	mm="10";;
	'Nov' | 'November')	mm="11";;
	'Dec' | 'December')	mm="12";;
	'*')			mm="";;
esac

# assign day
dd=$2

# assign year
if [ `echo $3 | wc -c` -ge 4 ]
then
	yy=`echo "$3" | cut -c3,4`
else 
	yy=$3
fi

# validate full date
valdate $mm/$dd/$yy
