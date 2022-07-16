//	Copyright (c) 1988 AT&T
//	All Rights Reserved 
//	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
//	The copyright notice above does not evidence any 
//	actual or intended publication of such source code.

#ident	"@(#)sdb:libutil/common/set_watch.C	1.1"
#include	"utility.h"
#include	"Process.h"
#include	"Interface.h"

int
set_watch( Process * process, char * estring )
{
	if ( process == 0 )
	{
		printe("internal error: ");
		printe("process pointer was zero\n");
		return 0;
	}
	else if ( process->set_wpt( estring ) == 0 )
	{
		printe("failure to set watchpoint\n");
		return 0;
	}
	else
	{
		return 1;
	}
}
