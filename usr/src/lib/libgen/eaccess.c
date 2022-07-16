/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libgen:eaccess.c	2.2.4.2"
/*	Determine if the effective user id has the appropriate permission
	on a file.  Modeled after access(2).
	amode:
		00	just checks for file existence.
		04	checks read permission.
		02	checks write permission.
		01	checks execute/search permission.
		other bits are ignored quietly.
*/

#ifdef __STDC__
	#pragma weak eaccess = _eaccess
#endif
#include "synonyms.h"

#include	<errno.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	"libgen.h"

extern int	errno;
extern int	access();
extern int	stat();
extern gid_t	getegid();
extern uid_t    geteuid();


int
eaccess( path, amode )
const char		*path;
register int	amode;
{
	uid_t euid;
	struct stat	s;
	/* check for access errno other than EACCES */
	if(access(path, amode) && errno != EACCES) 
		return  -1;
	if (stat( path, &s ) == -1 )
		return -1;

	amode <<= 6;
	if(!amode || (euid = geteuid()) == 0)
		/* file exists or superuser */
		return  0;

	if( euid == s.st_uid  &&  (amode & s.st_mode) == amode ) 
		return  0;		/* access permitted by owner mode */
	amode >>= 3;
	if(getegid() == s.st_gid  &&  (amode & s.st_mode) == amode ) 
		return  0;		/* access permitted by group mode */

	amode >>= 3;
	if( (amode & s.st_mode) == amode ) 
		return  0;	 	/* access permitted by "other" mode */
	return -1;
}
