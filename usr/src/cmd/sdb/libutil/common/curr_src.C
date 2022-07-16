//	Copyright (c) 1988 AT&T
//	All Rights Reserved 
//	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
//	The copyright notice above does not evidence any 
//	actual or intended publication of such source code.

#ident	"@(#)sdb:libutil/common/curr_src.C	1.2"
#include	"Interface.h"
#include	"Process.h"
#include	"utility.h"

char *
current_src( Process * process, long * line )
{
	if ( process == 0 )
	{
		printe("internal error: ");
		printe("null pointer to current_src()\n");
		return 0;
	}
	if ( line != 0 )
	{
		*line = process->current_line();
	}
	return process->current_srcfile;
}
