#ident	"@(#)lp.admin:printers/requests/Form.move	1.4"
#######################################################
#
#       Module Name: Form.move
#
#######################################################

form=Move Print Requests to a New Printer

#help=OPEN TEXT $INTFBASE/Text.oamhelp $PRINTSERV/HELP/status/requests.help "Displays Status of Print Requests"



`indicator -w; set -l name_p="/tmp/lp.rp$VPID";
	set -l name_px="/tmp/lp.rpx$VPID";
	set -l name_r="/tmp/lp.rr$VPID";
	set -l name_rx="/tmp/lp.rrx$VPID";
     lpstat -oall -l | tr -s ',' ' ' | fmlgrep 'queued' | fmlcut -d' ' -f3 > $name_px;
	ls /etc/lp/printers > $name_p;
	echo "all" >> $name_px;
	echo "" >> $name_px;
        lpstat -oall | fmlcut -d' ' -f1 > $name_r;
	set -l name_rx=$name_r;
if [ -n "$name_rx" ];
then
	echo "" >> $name_rx;
	echo "all" >> $name_rx;
	echo "init=true";
else
	echo "init=false";
	message -b "There are no print requests available";
fi;
	`


done=`message -w "Moving print requests to $F3";
	if [ ( -n $F1 -a $F1 != 'all' ) -a ( -z $F2 -o "$F2" = 'all' ) ];
	then
	echo "$F1" | tr -s ',' ' ' | set -l plist;
	shell "
	for p in $plist
	do
	lpmove \$p $F3 ;
	done
	"  2> $error;
	if [ -s $error ];
	then OPEN TEXT $OBJ_DIR/Text.error;
	fi;
	fi;
	if [ -n $F2 -a $F2 != "all" ];
	then 
	shell "lpmove "$F2" $F3" 2> $error;
	if [ -s $error ];
	then OPEN TEXT $OBJ_DIR/Text.error;
	fi;
	fi;
     	if [  -z $F1 -a $F2 = 'all' ];
	then
	readfile $name_r | tr '\012' ' ' | set -l plist;
	shell "lpmove "$plist" $F3" 2> $error;
	if [ -s $error ];
	then OPEN TEXT $OBJ_DIR/Text.error;
	fi;
	fi;
	if [ $F1 = 'all' -a  ( $F2 = 'all' -o -z $F2 ) ) ];
	then
	readfile $name_r | tr '\012' ' ' | set -l plist;
	shell "lpmove "$plist" $F3" 2> $error;
	if [ -s $error ];
	then OPEN TEXT $OBJ_DIR/Text.error;
	fi;
	fi;
	message "";
        `update

close=`/usr/bin/rm -f $name_p;
	/usr/bin/rm -f $name_px;
	/usr/bin/rm -f $name_r;
	/usr/bin/rm -f $name_rx;
	unset -l do_all -l name_p -l -l name_r name_valid -l bad_one`

name=Current Printers:
nrow=1
ncol=1
frow=1
fcol=23
rows=1
columns=52
fieldmsg="Enter the printers with requests to be moved" 
rmenu=OPEN MENU $OBJ_DIR/Menu.pr.q.ch
choicemsg="MARK current printers then press ENTER"

valid=`indicator -w;
	unset -l bad_one;
	if [ -z "$F1" ];
	then
		set -l name_valid=true;
		true;
	else
		set -l name_valid=true;
	fi;
	echo "$F1" | tr " ," "[\012*]" | regex -e '^(.*)$0$'
	'`fmlgrep "^$m0\$" $name_px || set -l bad_one="$m0" -l name_valid=false`' > /dev/null`$name_valid
invalidmsg="$bad_one invalid printer - press [CHOICES] for selection"


name=Request ID number(s):
nrow=2
ncol=1
frow=2
fcol=23
rows=1
columns=52
value=all
scroll=true
rmenu=OPEN MENU $OBJ_DIR/Menu.rq.ch "$F1"
choicemsg="MARK requests then press ENTER"
fieldmsg="Enter the request-ids to move."

valid=`indicator -w;
	unset -l bad_one;
	if [ -z "$F2" -a -n "$F1" ];
	then
		set -l no_value=false -l name_valid=true;
		true;
	fi;
	if [ -z $F2 -a -z $F1 ];
	then 
		set -l no_value=true -l name_valid=false;
	fi;
	if [ -n $F2 -a -z $F1 ];
	then 
		set -l no_value=false -l name_valid=true;
	fi;
	echo "$F2" | tr " ," "[\012*]" | regex -e '^(.*)$0$'
	'`fmlgrep "^$m0\$" $name_rx || set -l bad_one="$m0" -l name_valid=false`' > /dev/null`$name_valid
invalidmsg=`if [ "$no_value" = "true" ];
	then
	    echo "You must enter a request-id or an original printer";
	else
	    echo "$bad_one Invalid print request - press [CHOICES] for selection";
	fi;`



name=New Printer:

nrow=3
ncol=1
frow=3
fcol=18
rows=1
columns=57
menuonly=true
rmenu={ `ls /etc/lp/printers` }
choicemsg="Select new printer then press ENTER"
fieldmsg="Enter the new printer then press [SAVE] to move requests"
valid=`indicator -w;
	unset -l bad_one;
	if [ -z "$F3" ];
	then
		set -l no_value=true -l name_valid=false;
	else
		set -l no_value=false -l name_valid=true;
	fi;
	echo "$F3" | tr " ," "[\012*]" | regex -e '^(.*)$0$'
	'`fmlgrep "^$m0\$" $name_p || set -l bad_one="$m0" -l name_valid=false`' > /dev/null`$name_valid
invalidmsg=`if [ "$no_value" = "true" ];
	then
		echo "Must have destination printer to move print requests";
	else
		echo "$bad_one invalid printer - press [CHOICES] for selection";
	fi`

