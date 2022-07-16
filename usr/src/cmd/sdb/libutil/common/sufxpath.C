//	Copyright (c) 1988 AT&T
//	All Rights Reserved 
//	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
//	The copyright notice above does not evidence any 
//	actual or intended publication of such source code.

#ident	"@(#)sdb:libutil/common/sufxpath.C	1.1"

#include	"utility.h"
#include	<string.h>

extern char *	global_path;

int
suffix_path( char * string )
{
	char *	p2;

	if ( string == 0 )
	{
		p2 = global_path;
	}
	else if ( global_path == 0 )
	{
		p2 = new char[ ::strlen(string) + 1 ];
		::strcpy( p2, string );
	}
	else
	{
		p2 = new char[ ::strlen(global_path ) + ::strlen(string) + 2 ];
		::strcpy( p2, global_path );
		::strcat( p2, ":" );
		::strcat( p2, string );
		delete global_path;
	}
	global_path = p2;
	return 1;
}
