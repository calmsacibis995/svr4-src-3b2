//	Copyright (c) 1988 AT&T
//	All Rights Reserved 
//	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
//	The copyright notice above does not evidence any 
//	actual or intended publication of such source code.

#ident	"@(#)sdb:libutil/common/eval_expr.C	1.1"
#include	"utility.h"
#include	"Process.h"
#include	"Interface.h"

int
evaluate_expr( Process * process, char * estring, char * fmt )
{
	if ( process == 0 )
	{
		printe("internal error: ");
		printe("null process pointer to evaluate_expr()\n");
		return 0;
	}
	else if ( estring == 0 )
	{
		printe("internal error: ");
		printe("null expression string to evaluate_expr()\n");
		return 0;
	}
	else
	{
		return process->evaluate_expr( estring, fmt );
	}
}
