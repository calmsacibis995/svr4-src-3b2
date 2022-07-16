#!/sbin/sh
#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)lp.admin:printers/printers/add/mkform.sh	1.1"
echo name=
echo nrow=1
echo ncol=1
echo from=1
echo fcol=11
echo rows=1
echo columns=14
echo value=$1
echo inactive=TRUE

	index=0
	nrow=3
	ncol=1

	cs=`tput -T $lp_prtype csnm $index 2> /dev/null`
	while [ $? -eq 0 -a -n "$cs" -a $index -lt 64 ]
	do
	    echo "name=cs$index:\nnrow=$nrow\nncol=$ncol"
	    echo "frow=$nrow\nfcol=`expr $ncol + 5`"
	    echo "rows=1\ncolumns=14"
	    echo "value=\`echo cs$index=$cs >> $saveinit;echo $cs\`"
	    echo "scroll=true"
	    index=`expr $index + 1`
	    echo "valid=\`regex -v \"\$F`expr $index + 1`\" \"^[_a-zA-Z0-9]{1,14}\$\"\`"
	    echo "invalidmsg='Invalid entry: Alpha-numeric characters and \"_\" are valid.'"
	    echo "";
	    cs=`tput -T $lp_prtype csnm $index 2> /dev/null`
	    ncol=`expr $ncol + 19`
	    if [ $ncol -gt 75 ]
	    then
		ncol=1
		nrow=`expr $nrow + 1`
	    fi
	done

	exit $index
