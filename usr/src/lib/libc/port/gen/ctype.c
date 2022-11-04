/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/ctype.c	1.12"
#include "synonyms.h"
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#define CCLASS_ROOT	"/usr/lib/chrclass/"/* root for character classes */
#define LEN_CC_ROOT	14		/* length of CCLASS_ROOT */
#define DEFL_CCLASS	"ascii"		/* default character class */
#define SZ		514

static char first_call = 1;

int
_setchrclass(ccname)
const char *ccname;
{
	char pathname[128];
	char my_ctype[SZ];		/* local copy */
	register int fd, ret = -1;

       /*
	* If no character class name supplied, use the CHRCLASS env. variable.
	*/
	if (ccname == 0 && ((ccname = getenv("CHRCLASS")) == 0 || ccname[0] == '\0'))
	{
		/*
		* If this is the first call to setchrclass() then there is
		* nothing else to do here.  If this is a subsequent call
		* to setchrclass() and if _ctype[] was set to anything but
		* the default, the pathname[] array would have been changed.
		* Thus, in this case, actually do the reset to "ascii".
		*/
		if (first_call)
			return (0);
		ccname = DEFL_CCLASS;
	}
	first_call = 0;
	(void)strcpy(pathname, CCLASS_ROOT);
	(void)strcpy(&pathname[LEN_CC_ROOT], ccname);
	if ((fd = open(pathname, O_RDONLY)) >= 0)
	{
		if (read(fd, my_ctype, SZ) == SZ)
		{
			/*
			* Update _ctype[] only after everything works.
			*/
			(void)memcpy(_ctype, my_ctype, SZ);
			ret = 0;
		}
		(void)close(fd);
	}
	return (ret);
}
