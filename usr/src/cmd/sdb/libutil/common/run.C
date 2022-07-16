//	Copyright (c) 1988 AT&T
//	All Rights Reserved 
//	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
//	The copyright notice above does not evidence any 
//	actual or intended publication of such source code.

#ident	"@(#)sdb:libutil/common/run.C	1.6"
#include	"utility.h"
#include	"Process.h"
#include	"Interface.h"

int
run( Process * process, int clearsig, Location * location, int count )
{
	Iaddr	addr;

	if ( process == 0 )
	{
		printe("internal error: ");
		printe("process pointer was zero\n");
		return 0;
	}
	else if ( get_addr( process, location, addr ) == 0 )
	{
		printe("could not run to specified location\n");
		return 0;
	}
	else
	{
		return process->run( clearsig, addr, count, 0 );
	}
}
