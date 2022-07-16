//	Copyright (c) 1988 AT&T
//	All Rights Reserved 
//	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
//	The copyright notice above does not evidence any 
//	actual or intended publication of such source code.

#ident	"@(#)sdb:sdb/common/do_assoc.C	1.3"

#include	"Assoccmds.h"
#include	"utility.h"
#include	"Interface.h"
#include	"Process.h"

extern int parse(char *);

int
do_assoccmds()
{
	Assoccmds *	a;
	char *		p;

	while ( (a = dequeue()) != 0 )
	{
		a->reset();
		while ( (p = a->get_cmd()) != 0 )
		{
			parse(p);
		}
		dispose_assoc( a );
	}
	return 1;
}
