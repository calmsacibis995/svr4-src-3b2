/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nlp:lib/forms/putform.c	1.8"
/* LINTLIBRARY */
# include	<stdio.h>
# include	<errno.h>
# include	<string.h>
# include	"lp.h"
# include	"form.h"

extern char **environ;
#if	defined(__STDC__)
char		*getformdir ( const char *, int );
int		wrform ( FORM *, FILE * );
#else	/* defined(__STDC__) */
extern char	*getformdir();
extern int	wrform();
#endif	/* defined(__STDC__) */

int putform(form,formp,alertp,alignfilep)
char *form;
FORM *formp;
FALERT *alertp;
FILE **alignfilep;
{
    FILE *formfopen();
    FILE *fp;
    int wralign();
    char *path;
    int size;

    if (!Lp_A_Forms)	
	getadminpaths(LPUSER);
    if (!Lp_A_Forms)
	return(-1);

    if (STREQU(NAME_ALL,form))
    {
	errno = EINVAL;
	return(-1);
    }

    if (formp != NULL )
    {
	if ((fp = formfopen(form,DESCRIBE,"w",1)) == NULL)
	    return(-1);
	else
	{
    	    (void) wrform(formp,fp);
    	    (void) close_lpfile(fp);
	}
	if (formp->comment == NULL)
	{
	    if (path = makepath(getformdir(form, 0),ALIGN_PTRN,(char *) 0))
	    {
		Unlink(path);		/* MR bl87-29602 */
		free(path);
	    }
	}
	else
	{
	    if ((fp = formfopen(form,COMMENT,"w",0)) == NULL)
	    	return(-1);
	    (void) fprintf(fp,"%s",formp->comment);
	    (void) close_lpfile(fp);
	}
    }

    if (alertp != NULL )
    {
	
	if (putalert(Lp_A_Forms,form,alertp) == -1)
	    return(-1);
    }

    if (alignfilep != NULL )
    {
	if ((fp = formfopen(form,ALIGN_PTRN,"w",1)) == NULL)
	    return(-1);
	else
	{
	    size = wralign(fp,*alignfilep);
	    (void) close_lpfile(fp);
	    if (!size)
	    {
		if (path = makepath(getformdir(form, 0),ALIGN_PTRN,(char *) 0))
		{
		    Unlink(path);
		    free(path);
		}
	    }
	}
    }
    
    return(0);
}

FILE *formfopen(formname,file,type,dircreat)
char *formname;
char *file;
char *type;
register int dircreat;
{
    FILE *fp;
    char *filepath,*pathptr;

	/* If positive form directory is crteated if it doesn't exist */
	if (dircreat)
	    pathptr = getformdir(formname, 1);
	else {
    	    if ((pathptr = getformdir(formname, 0)) == NULL)
	    {
		if (errno == ENOENT) errno = ENOTDIR;
		return(NULL);
		}
	    }
    if ((filepath = makepath(pathptr,file,(char *)0)) == NULL)
	    return(NULL);

    	if ( (fp = open_lpfile(filepath,type, 
	     (STREQU(file,ALIGN_PTRN) ) ? MODE_NOREAD : MODE_READ))
		== NULL) {
	    free(filepath);
	    return(NULL);
	    }
	free(filepath);
	return(fp);
}
