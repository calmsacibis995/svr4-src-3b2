#ident	"@(#)oamintf:machinemgmt/Text.boot	1.3"

################################################################################
#	Module Name: Text.boot
################################################################################
title="Boot Defaults"

framemsg=`readfile $INTFBASE/conf.msg`

text="
You may specify the default file for manual load, the
device for auto load, or both.

Typical files to be loaded are \"/unix\", a fully configured
UNIX, or \"/etc/system\", a system specification file.  The
latter implies a self-configuration boot, i.e. the version
of UNIX to be used will be generated as the system loads.
Note that the file name is not validated until boot time
so make sure it is correct.

Typical devices to be used for auto load are hard disks,
e.g. HD30.  Note that the peripheral floppy cannot be
used for auto load purposes."

rows=15
columns=70

altslks

name=CONT
button=11
action=`run /etc/fltboot`

name=CANCEL
button=14
action=CLEANUP
