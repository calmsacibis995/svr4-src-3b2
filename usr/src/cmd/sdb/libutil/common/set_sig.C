//	Copyright (c) 1988 AT&T
//	All Rights Reserved 
//	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
//	The copyright notice above does not evidence any 
//	actual or intended publication of such source code.

#ident	"@(#)sdb:libutil/common/set_sig.C	1.4"
#include	"prioctl.h"
#include	"utility.h"
#include	"Process.h"
#include	"Interface.h"

int
set_signal_set( Process * process, sigset_t sigset )
{
	if ( process == 0 )
	{
		printe("internal error: ");
		printe("process pointer was zero\n");
		return 0;
	}
	else
	{
		return process->set_sig_catch( sigset );
	}
}
