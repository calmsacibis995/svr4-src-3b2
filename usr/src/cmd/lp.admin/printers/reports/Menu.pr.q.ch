#ident	"@(#)lp.admin:printers/reports/Menu.pr.q.ch	1.1"

menu=Choices
multiselect=true
framemsg="MARK choices then press ENTER"


`set -l name_1="/tmp/lp.n1$VPID";
lpstat -oall -l | tr -s ',' ' ' | fmlgrep 'queued' | fmlcut -d' ' -f3  > $name_1;
if [ -s $name_1 ];
then
	echo "all" >> $name_1;
else
	echo "init=false";
	message -b "There are no printers with print requests";
	rm  $name_1;
fi`

close=`rm  $name_1;
	unset -l $name_1`

done=`getitems " "|set -l Form_Choice`close

`/usr/bin/sort -u $name_1 | regex '^(.*)$0$' 'name=$m0'`
