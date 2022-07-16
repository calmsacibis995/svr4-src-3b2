//	Copyright (c) 1988 AT&T
//	All Rights Reserved 
//	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
//	The copyright notice above does not evidence any 
//	actual or intended publication of such source code.

#ident	"@(#)sdb:libutil/common/disp_break.C	1.1"
#include	"utility.h"
#include	"Process.h"
#include	"Interface.h"

int
display_breaks( Process * process )
{
	if ( process == 0 )
	{
		printe("internal error : null Process pointer\n");
		return 0;
	}
	else
	{
		return process->display_bkpts();
	}
}
