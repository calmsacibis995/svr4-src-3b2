//	Copyright (c) 1988 AT&T
//	All Rights Reserved 
//	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
//	The copyright notice above does not evidence any 
//	actual or intended publication of such source code.

#ident	"@(#)sdb:libutil/common/regs.C	1.7"
#include "Interface.h"
#include "utility.h"
#include "Process.h"

int
printregs( Process * proc, int num_per_line )
{
	if ( !proc || proc->is_proto() ) {
		printe("no process\n");
		return 0;
	}
	else
	{
		return proc->display_regs( num_per_line );
	}
}
