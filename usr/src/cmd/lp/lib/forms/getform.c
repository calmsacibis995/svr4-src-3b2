/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nlp:lib/forms/getform.c	1.12"
/* LINTLIBRARY */

# include	<stdio.h>
# include	<fcntl.h>
# include	<string.h>
# include	<sys/types.h>
# include	<errno.h>
# include	<stdlib.h>

# include	"lp.h"
# include	"form.h"

long		next_read = 0;

#if	defined(__STDC__)
char *		getformdir ( char *, int );
int		scform ( char *, FORM *, FILE *, short, int * );
#else
char *		getformdir();
int		scform();
#endif

#if	defined(__STDC__)
int getform ( char * form, FORM * formp, FALERT * alertp,
	      FILE ** alignfilep )
#else
int getform (form, formp, alertp, alignfilep)
    char	*form;
    FORM	*formp;
    FALERT	*alertp;
    FILE	**alignfilep;
#endif
{
    FILE		*formfopen();
    FILE		*fp;
    DIR			*dirp;
    FALERT		*returned,*getalert();
    struct dirent	*nextent;
    struct stat		statbuf;
    char		*pathptr,*filepath;
    char		*name;
    int			opttag[NFSPEC];
    int			fd;

    if (!Lp_A_Forms)
	getadminpaths(LPUSER);

    if (!Lp_A_Forms)
	return(-1);
    
    if (STREQU(NAME_ALL,form))
    {
	/*
	**	If getform was invoked with the special name "all", the
	**	formp argument must not be null.  The name of the form
	**	being returned is stored in formp and retrieving form
	**	data without knowing which form it is associated with
	**	is useless, so using getform("all", NULL, ...) is illegal.
	*/

	if (formp == NULL)
	{
	    errno = EINVAL;
	    lp_errno = LP_ENULLPTR;
	    return(-1);
	}

	if ((dirp = Opendir(Lp_A_Forms)) == NULL)
	    return(-1);

	if (next_read)			/* TRUE if last call was "all" */
	    Seekdir(dirp, next_read);

	do
	    nextent = Readdir(dirp);	/* Skip over UNIX files "." and ".." */
	while (STREQU(nextent->d_name,".") || STREQU(nextent->d_name,".."));

	if (nextent == NULL)
	{
	   Closedir(dirp);

	   if (!next_read)		/* Not called with "all" last time */
		errno = EFAULT;

	   return(-1);
	}

	formp->name = strdup(nextent->d_name);
	name = strdup(nextent->d_name);

	next_read = Telldir(dirp);	/* save for next call with "all" */

	Closedir(dirp);
    }
    else
	name = strdup(form);

    if (formp != NULL )
    {
	if ((fp = formfopen(name,DESCRIBE,"r",0)) == NULL)
	{
	    if (errno == ENOENT)
		errno = EBADF;

	    if (errno == ENOTDIR)
		errno = ENOENT;

	    return(-1);
	}


	/*
	**	Null out comment and alignment pattern content type
	**	pointers in case the form does not have a comment or
	**	alignment pattern. This is necesary since the comment
	**	and alignment pattern are stored in seperate files
	**	from the describe file and formp is generally used
	**	only in conjunction with describe file activities. The
	**	comment and content pointers are necessary for adding
	**	a form primarily.
	*/

	formp->comment = formp->conttype = (char *)NULL;

    	if (scform(name, formp, fp, 1, opttag) == -1)
	   return(-1);

    	(void) close_lpfile(fp);

    	if ((pathptr = getformdir(name, 0)) == NULL)
	    return(-1);

	if ((filepath = makepath(pathptr, COMMENT, (char *)0)) == NULL)
	    return(-1);

	if ( (fd = Open(filepath,O_RDONLY, 0)) != -1)
	{
	    if (Fstat(fd,&statbuf) == -1)
	    	return(-1);

	    if (statbuf.st_size)
	    {
	        formp->comment = malloc((unsigned int)statbuf.st_size + 1);

		if (formp->comment == NULL)
		    return(-1);

	        Read(fd, formp->comment, (unsigned int)statbuf.st_size);
	        formp->comment[statbuf.st_size] = (char)NULL;
	    }
	    Close(fd);	
	}
	free(filepath);
    }

    if (alertp != NULL)
    {
	if ((returned = getalert(Lp_A_Forms,name)) == NULL)
	{
	    if (errno == ENOENT)
	    {
	    	alertp->shcmd = (char *)NULL;
		alertp->Q = alertp->W = -1;
	    }
	    else
	    {
		/* If form does not exist have ENOENT set */
		if (errno == ENOTDIR)
		    errno = ENOENT;
		return(-1);
	    }
	}
	else
	{
	    alertp->shcmd = strdup(returned->shcmd);
	    alertp->Q = returned->Q;
	    alertp->W = returned->W;
	}
    }

    if (alignfilep != NULL )
	*alignfilep = formfopen(name,ALIGN_PTRN,"r",0);
    free(name);
    return(0);
}
