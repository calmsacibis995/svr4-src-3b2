/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nlp:lib/forms/delform.c	1.9"
/* LINTLIBRARY */

# include	<stdio.h>
# include	<errno.h>
# include	<sys/types.h>
# include	<string.h>
# include	<limits.h>
# include	"lp.h"
# include	"form.h"

#if	defined(__STDC__)
char *			getformdir ( char *, int );
int			formtype ( char * name );
#else
char *			getformdir();
int			formtype();
#endif

int delform(formname)
char *formname;
{
    register int type;
    char *formpath;
    char savedir[PATH_MAX + 1];

    if (!Lp_A_Forms)
	getadminpaths(LPUSER);
    if (!Lp_A_Forms)
    {
	errno = ENOMEM;
	return(-1);
    }
    
    type = formtype(formname);
    switch (type) {
    case NO_FORM:   errno = ENOENT;
		    return(-1);
    case ALLFORMS: /* remove all forms in LP forms database */
		    (void)getcwd(savedir, PATH_MAX);
	    	    if (Chdir(Lp_A_Forms) != 0) {
			errno = EACCES;
			return(-1);
			}
	    	    if (system("rm -r -f *") < 0) {
			errno = EACCES;
			return(-1);
			}
		    (void) Chdir(savedir);

		    break;
    case REG_FORM:
		    (void)getcwd(savedir, PATH_MAX);
    		    formpath = getformdir(formname, 0);
	    	    if (Chdir(formpath) != 0) {
			errno = EACCES;
			return(-1);
			}
	    	    if (system("rm -r -f *") < 0) {
			errno = EACCES;
			return(-1);
			}
		    (void) Chdir(savedir);
	    	    if (Rmdir(formpath) != 0) {
			errno = EACCES;
			return(-1);
			}
		    break;
	}
    return(0);
}

int formtype(formname)
char *formname;
{
    char *formpath;

    if (STREQU(NAME_ALL,formname))
	return(ALLFORMS);
    formpath = getformdir(formname, 0);
    if (Access(formpath,0) != 0)
	return(NO_FORM);
    return(REG_FORM);
}
