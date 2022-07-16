#ident	"@(#)face:src/oam/programs/Menu.mod	1.2"

menu="Modify Global Programs"

help=open TEXT $INTFBASE/Text.itemhelp 'menu:F1'

framemsg="Move to an item with arrow keys and press ENTER to select the item."

`fmlgrep '^vmsys:' /etc/passwd | fmlcut -f6 -d: | set -e VMSYS;
$VMSYS/bin/listserv -m VMSYS`
