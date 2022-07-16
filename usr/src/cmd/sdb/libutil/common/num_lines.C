//	Copyright (c) 1988 AT&T
//	All Rights Reserved 
//	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
//	The copyright notice above does not evidence any 
//	actual or intended publication of such source code.

#ident	"@(#)sdb:libutil/common/num_lines.C	1.1"
#include	"Interface.h"
#include	"SrcFile.h"
#include	"utility.h"

long
num_lines( SrcFile * srcfile )
{
	if ( srcfile == 0 )
	{
		printe("internal error: ");
		printe("null pointer to src_text()\n");
		return 0;
	}
	else
	{
		return srcfile->num_lines();
	}
}
