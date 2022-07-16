/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)lp:lib/systems/delsystem.c	1.2"
/* LINTLIBRARY */

# include	<string.h>
# include	<stdlib.h>
# include	<errno.h>

# include	"lp.h"
# include	"systems.h"

# define	SEPCHARS	":\n"

/**
 ** delsystem()
 **/

#if	defined(__STDC__)
int delsystem ( const char * name )
#else
int delsystem ( name )
char	*name;
#endif
{
    FILE	*fpi;
    FILE	*fpo;
    char	*cp;
    char	*file;
    char	buf[BUFSIZ];
    char	c;
    int		all = 0;
    int		errtmp;

    if ((file = tempnam(ETCDIR, "lpdat")) == NULL)
    {
	errno = ENOMEM;
	return(-1);
    }

    if ((fpi = open_lpfile(Lp_NetData, "r", MODE_READ)) == NULL)
    {
	free(file);
	return(-1);
    }

    if ((fpo = open_lpfile(file, "w", MODE_READ)) == NULL)
    {
	errtmp = errno;
	(void) close_lpfile(fpi);
	free(file);
	errno = errtmp;
	return(-1);
    }

    if (STREQU(NAME_ALL, name))
	all = 1;

    while (fgets(buf, BUFSIZ, fpi) != NULL)
    {
	if (*buf != '#' && *buf != '\n')
	    if ((cp = strpbrk(buf, SEPCHARS)) != NULL)
	    {
		if (all)
		    continue;
		
		c = *cp;
		*cp = '\0';
		if (STREQU(name, buf))
		    continue;
		*cp = c;
	    }

	if (fputs(buf, fpo) == EOF)
	{
	    errtmp = errno;
	    (void) close_lpfile(fpi);
	    (void) close_lpfile(fpo);
	    (void) Unlink(file);
	    free(file);
	    errno = errtmp;
	    return(-1);
	}
    }

    (void) close_lpfile(fpi);
    (void) close_lpfile(fpo);
    (void) _Rename(file, Lp_NetData);
    free(file);
    return(0);
}
