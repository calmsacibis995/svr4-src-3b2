#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

#!/bin/sh
#ident	"@(#)sdb:cfront/scratch/bsd.sed	1.1"
echo "Fixing _iobuf structures:"
for f in */*..c
do
	echo $f
        sed -e '/__iobuf__base/s//&; int __iobuf__bufsiz/'  \
	-e '/char __iobuf__flag/s//short __iobuf__flag/' \
	-e '/_ctype/s//_ctype_/g' $f > temp$$
	mv temp$$ $f
done
