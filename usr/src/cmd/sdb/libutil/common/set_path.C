//	Copyright (c) 1988 AT&T
//	All Rights Reserved 
//	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
//	The copyright notice above does not evidence any 
//	actual or intended publication of such source code.

#ident	"@(#)sdb:libutil/common/set_path.C	1.4"

#include	<string.h>

char *	global_path = 0;
int	pathage = 0;

int
set_path( char * path )
{
	if ( global_path != 0 ) delete global_path;
	global_path = new char[ ::strlen( path ) + 1 ];
	::strcpy( global_path, path );
	++pathage;
	return 1;
}
