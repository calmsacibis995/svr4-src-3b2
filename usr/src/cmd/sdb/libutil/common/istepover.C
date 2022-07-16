//	Copyright (c) 1988 AT&T
//	All Rights Reserved 
//	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
//	The copyright notice above does not evidence any 
//	actual or intended publication of such source code.

#ident	"@(#)sdb:libutil/common/istepover.C	1.4"
#include	"utility.h"
#include	"Process.h"
#include	"Interface.h"

int
instr_step( Process * process, int clearsig, Location * loc, int cnt, int talk )
{
	Iaddr	addr;

	if ( process == 0 )
	{
		printe("internal error: ");
		printe("process pointer was zero\n");
		return 0;
	}
	else if ( get_addr( process, loc, addr ) == 0 )
	{
		printe("could not step to specified location\n");
		return 0;
	}
	else
	{
		return process->instr_step_over( clearsig, addr, cnt, talk );
	}
}
