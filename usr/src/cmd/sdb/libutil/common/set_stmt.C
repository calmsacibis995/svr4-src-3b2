//	Copyright (c) 1988 AT&T
//	All Rights Reserved 
//	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
//	The copyright notice above does not evidence any 
//	actual or intended publication of such source code.

#ident	"@(#)sdb:libutil/common/set_stmt.C	1.2"
#include	"Interface.h"
#include	"Process.h"
#include	"utility.h"

int
set_current_src( Process * process, char * filename, long line )
{
	if ( process == 0 )
	{
		printe("internal error: ");
		printe("null pointer to set_current_src()\n");
		return 0;
	}
	else
	{
		return process->set_current_stmt( filename, line );
	}
}
