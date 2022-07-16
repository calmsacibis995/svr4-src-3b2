//	Copyright (c) 1988 AT&T
//	All Rights Reserved 
//	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
//	The copyright notice above does not evidence any 
//	actual or intended publication of such source code.

#ident	"@(#)sdb:libutil/common/src_text.C	1.1"
#include	"Interface.h"
#include	"SrcFile.h"
#include	"utility.h"

char *
src_text( SrcFile * srcfile, long line )
{
	if ( srcfile == 0 )
	{
		printe("internal error: ");
		printe("null pointer to num_lines()\n");
		return 0;
	}
	else
	{
		return srcfile->line( line );
	}
}
