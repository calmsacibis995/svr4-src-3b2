//	Copyright (c) 1988 AT&T
//	All Rights Reserved 
//	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
//	The copyright notice above does not evidence any 
//	actual or intended publication of such source code.

#ident	"@(#)sdb:libutil/common/set_break.C	1.8"
#include	"utility.h"
#include	"Process.h"
#include	"Interface.h"

int
set_break( Process * process, Location * location, Assoccmds * a, int announce )
{
	Iaddr	addr;

	if ( process == 0 )
	{
		printe("internal error : null Process pointer\n");
		return 0;
	}
	else if ( get_addr( process, location, addr ) == 0 )
	{
		printe("no code at specified location\n");
		return 0;
	}
	else
	{
		return process->set_bkpt( addr, a, announce );
	}
}
