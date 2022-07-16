//	Copyright (c) 1988 AT&T
//	All Rights Reserved 
//	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
//	The copyright notice above does not evidence any 
//	actual or intended publication of such source code.

#ident	"@(#)sdb:libutil/common/assoc.C	1.2"
#include	"Assoccmds.h"
#include	"utility.h"
#include	"Interface.h"

Assoccmds *
new_assoc()
{
	return new Assoccmds;
}

int
add_assoc( Assoccmds * a, char * p )
{
	if ( a == 0 )
	{
		return 0;
	}
	else if ( p == 0 )
	{
		return 0;
	}
	else if (*p == '\n') 
	{
		return 0;
	}
	else
	{
		a->add_cmd( p );
		return 1;
	}
}
