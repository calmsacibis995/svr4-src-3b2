/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nlp:lib/forms/rdform.c	1.13"
/* LINTLIBRARY */
# include	<stdio.h>
# include	<string.h>
# include	<ctype.h>
# include	<errno.h>
# include	<stdlib.h>

# include	"lp.h"
# include	"form.h"

int linenum;

#if	defined(__STDC__)
char		* getformdir ( const char * formname, int creatdir);
int		scform ( char *, FORM *, FILE *, short, int * );
int		putform ( char *, FORM *, FALERT *, FILE **);
int		syn_type ( char * );
#else
char		*getformdir();
int		scform();
int		putform();
int		syn_type();
#endif

int addform(form_name,formp,usrfp)
char *form_name;
FORM *formp;
FILE *usrfp; 
{
    FILE	*fp, *formfopen();
    int setopt[NFSPEC];
    int i;


    	if (scform(form_name,formp,usrfp,0,setopt) == -1)
	    return(-1);
	/* When a form is added an empty deny file is created as a default */
	fp = formfopen(form_name,DENYFILE,"w",1);
	(void) close_lpfile(fp);
	for ( i = 0; i < NFSPEC; i++)
	    if (setopt[i] == 0) {
		switch (i) {
		case FO_PLEN: 	formp->plen.val = DPLEN;
				formp->plen.sc = 0;
				break;
		case FO_PWID: 	formp->pwid.val = DPWIDTH;
				formp->pwid.sc = 0;
				break;
		case FO_NP: 	formp->np = DNP;
				break;
		case FO_LPI: 	formp->lpi.val = DLPITCH;
				formp->lpi.sc = 0;
				break;
		case FO_CPI: 	formp->cpi.val = DCPITCH;
				formp->cpi.sc = 0;
				break;
		case FO_CHSET: 	formp->chset = strdup(DCHSET);
				formp->mandatory = 0;
		  		break;
		case FO_RCOLOR: formp->rcolor = strdup(DRCOLOR);
				break;
		case FO_CMT: 	formp->comment = (char *)NULL;
				break;
		case FO_ALIGN: 	formp->conttype = (char *)NULL;
				break;
		}
		}
	return(0);
}

int chform(form_name,cform,usrfp)
char *form_name;
FORM *cform;
FILE *usrfp; /* *usrfp is NULL iff form_name is a registered lp form */
{
    int setopt[NFSPEC];

    return(scform(form_name,cform,usrfp,0,setopt));
}
	
int chfalert(old,new)
FALERT *new,*old;
{
    if (new->shcmd != NULL) {
	if (old->shcmd != NULL) free(old->shcmd);
	old->shcmd = strdup(new->shcmd);
	}
    if (new->Q != -1)
	old->Q = new->Q;
    else {
	/* Iff both alerts not specified(new alert!) Install alert default */
	if (old->Q == -1) old->Q = 1;
	}
    if (new->W != -1)
	old->W = new->W;
    else {
	/* Iff both alerts not specified(new alert!) Install alert default */
	if (old->W == -1) old->W = 0;
	}
    return(0);
}

int nscan(s,var)
char *s;
SCALED *var;
{
    char *ptr;

    if ( ( var->val = (float)strtod(s,&ptr)) <= 0) {
	errno = EBADF;
	lp_errno = LP_EBADSDN;
	return(-1);
	}

    if (ptr) {
	switch(*ptr) {
	case 0:
	case ' ':
	case '\n': var->sc = 0;
		   break;
	case 'c':
	case 'i':  var->sc = *ptr;
		   break;
	default:   errno = EBADF;
		   lp_errno = LP_EBADSDN;
		   return(-1);
	}
	}

    return(0);
}

void
stripnewline(line)
char *line;
{
    register char *p;
    
    p = line + strlen(line);
    while ( isspace(*--p));
    p[1] = 0;
}


/*
**	Function:	char * mkcmd_usr ( const char * cmd);
**
**	Purpose:	Return a string containing <cmd> a space and the
**			name of the user.
**
**	Warning:	This function returns a pointer to malloc() space
**			be careful to free it!
*/

#if	defined(__STDC__)
char * mkcmd_usr ( const char * const cmd )
#else	/* defined(__STDC__) */
char *mkcmd_usr(cmd)
char *cmd;
#endif	/* defined(__STDC__) */
{
    char *cmdstr,*login;
    
    if ((login = cuserid((char *)NULL)) == NULL)
	return(NULL);
    if ((cmdstr = malloc(strlen(cmd) + 1 + strlen(login) + 1)) == NULL)
	return(NULL);
    (void) strcpy(cmdstr,cmd);
    (void) strcat(cmdstr," ");
    (void) strcat(cmdstr,login);
    return(cmdstr);
}

/*
**	Function:	char * getformdir ( const char *	formname,
**					    int			creatdir ) ;
**
**	Purpose:	Return the path to the directory of <formname>,
**			making sure that it exists.  The forms directory
**			and the directory <formname> will be created if
**			necessary.
**
**	Warning:	The returned path is stored in static memory.
**			Subsequent call will overwrite it.
*/

#if	defined(__STDC__)
char * getformdir ( const char * formname, int creatdir)
#else	/* defined(__STDC__) */
char *
getformdir(formname, creatdir)
char *formname;
int creatdir;
#endif	/* defined(__STDC__) */
{
    static char	*cur_path = 0;
    static char	*cur_form = 0;

    if (!Lp_A_Forms)
	getadminpaths(LPUSER);
    if (!Lp_A_Forms)
	return(NULL);

    if (Access(Lp_A_Forms, 0) != 0)
	if (creatdir)
	    (void) mkdir_lpdir(Lp_A_Forms,0775);
	else
	    return(NULL);
    
    if (!cur_form || !STREQU(cur_form, formname))
    {
	if (cur_form)
	    free(cur_form);
	if (!(cur_form = strdup(formname)))
	    return(NULL);
	if (cur_path)
	    free(cur_path);
	if (!(cur_path = makepath(Lp_A_Forms,cur_form, (char *)0)))
	{
	    free(cur_form);
	    cur_form = NULL;
	    return(NULL);
	}
	if (Access(cur_path, 0) != 0)
	    if (creatdir)
		(void) mkdir_lpdir(cur_path, 0775);
	    else
	    {
		if (cur_form)
		    free(cur_form);
		if (cur_path)
		    free(cur_path);
		cur_form = NULL;
		cur_path = NULL;
	    }
    }
    return(cur_path);
}

#if	defined(__STDC__)
int		scform ( char * form_nm, FORM * formp, FILE *fp, short LP_FORM, int * opttag )
#else
int scform(form_nm,formp,fp,LP_FORM,opttag)
char *form_nm;
FORM *formp;
FILE *fp;
short LP_FORM;
int *opttag;
#endif
{
    char *retval;
    char line[BUFSIZ + 1];
    char *lineptr;
    char *p;
    long begblk,endblk,blklen;
    int i,j,n;
    int CMTEND,MATCH;

    linenum = 0;
    for ( i = 0; i < NFSPEC; opttag[i] = 0, i++);
    retval = fgets(line,BUFSIZ,fp);
    linenum++;
    while ( retval != NULL ) {
	CMTEND = MATCH = 0;
	for ( i = 0; i < NFSPEC && !MATCH; i++) { 
	    n = strlen(fspec[i]);
	    if ( (j = cs_strncmp(fspec[i],retval,n)) == 0)
		MATCH = 1;
	    }
	i--;

	if (!MATCH) {/* Unidentified input line */
	    errno = EBADF;
	    lp_errno = LP_EBADHDR; 
	    return(-1);
	    }
	opttag[i] = 1;
	lineptr = line;
	lineptr += n;
	while (*lineptr == ' ' || *lineptr == '\t')
	    lineptr++;
	switch (i) {
	    case FO_PLEN:   if (nscan(lineptr,&(formp->plen)) == -1)
				return(-1);
		    	    break;
	    case FO_PWID:   if (nscan(lineptr,&(formp->pwid)) == -1)
				return(-1);
		    	    break;
	    case FO_LPI:    if (nscan(lineptr,&(formp->lpi)) == -1)
				return(-1);
		    	    break;
	    case FO_CPI:    stripnewline(lineptr);
			    if (STREQU(lineptr, NAME_PICA)) {
				formp->cpi.val = 10;
				formp->cpi.sc = 0;
			    } else if (STREQU(lineptr, NAME_ELITE)) {
				formp->cpi.val = 12;
				formp->cpi.sc = 0;
			    } else if (STREQU(lineptr, NAME_COMPRESSED)) {
				formp->cpi.val = 9999;
				formp->cpi.sc = 0;
			    } else if (nscan(lineptr,&(formp->cpi)) == -1)
				return(-1);
		    	    break;
	    case FO_NP:     if ( ( formp->np = (int)strtol(lineptr,&p,10)) <= 0) {
				errno = EBADF;
				lp_errno = LP_EBADINT;
				return(-1);
				}
			    if (*p!= '\n') {
				errno = EBADF;
				lp_errno = LP_EBADINT;
				return(-1);
				}
		    	    break;
    
	    case FO_CHSET:  stripnewline(lineptr);
			    p = strchr(lineptr,',');
			    formp->mandatory = 0;
			    if (p) {
				do
				    *(p)++ = NULL;
				while (*p && isspace(*p));

		    	    	if (CS_STREQU(MANSTR,p))
				    formp->mandatory = 1;
				else {
				    errno = EBADF;
				    lp_errno = LP_EBADARG;
				    return(-1);
				    } 
				}
			    if (!syn_name(lineptr)) {
				errno = EBADF;
				lp_errno = LP_EBADNAME;
				return(-1);
				}
		            formp->chset = strdup(lineptr);		
		    	    break;
	    case FO_RCOLOR: stripnewline(lineptr);
			    if (!syn_name(lineptr)) {
				errno = EBADF;
				lp_errno = LP_EBADNAME;
				return(-1);
				}
		            formp->rcolor = strdup(lineptr);		
		    	    break;
	    case FO_CMT:    endblk = begblk = ftell(fp);
			    if (*lineptr != '\n') {
				errno = EBADF;
				lp_errno = LP_ETRAILIN;
				return(-1);
				}
    	    	    	    while ( (retval != NULL) && !CMTEND ) {
				endblk = ftell(fp);
    		    		retval = fgets(line,BUFSIZ,fp);
				linenum++;
				for ( j = 0; j < NFSPEC && !CMTEND; j++) { 
	    		    	n = strlen(fspec[j]);
	    		    	if ( cs_strncmp(fspec[j],retval,n) == 0)
			            CMTEND = 1;
	    		        }
			    }
			    if (!CMTEND)
			    	endblk = ftell(fp);
		    	    /* Store comment block */
		    	    blklen = endblk - begblk;
	    		    (void) fseek(fp,begblk,0);
		    	    if (blklen == 0) {
	    			formp->comment = NULL;
/*				errno = EBADF;
				lp_errno = LP_ENOCMT;
				return(-1);		/* MR bl87-29602 */
				}
		    	    else {
				formp->comment = malloc((unsigned) blklen + 1);
				if (formp->comment == NULL)
			    	    return(-1);
	    		    	(void) fread(formp->comment,1,(int) blklen,fp);
			    	formp->comment[(int) blklen] = '\0';
	    			}
			    linenum--;
		    	    break;
	    case FO_ALIGN:  stripnewline(lineptr);
			    if (!syn_type(lineptr)) {
				errno = EBADF;
				lp_errno = LP_EBADCTYPE;
				return(-1);
				}
		            formp->conttype = (*lineptr) ? strdup(lineptr) :
						strdup(DCONTYP);
			    if (formp->conttype == (char *)NULL)
				return(-1);
			    if (!LP_FORM) {
				begblk = ftell(fp);
/*				(void) fseek(fp,(long) 0,2);*/
				(void) fseek(fp,begblk,0);
/* MR bl87-29602 */
				if (putform(form_nm,NULL,NULL,&fp) == -1) {
				    free(formp->conttype);
				    formp->conttype = (char *)NULL;
				    return(-1);
				    }
				}
		    	    retval = NULL;
		    	    break;	
			
	    } /* End switch for tagged lines */


	if (retval != NULL) {
	    retval = fgets(line,BUFSIZ,fp);
	    linenum++;
	    }
	} /* End while for identifying input lines */
    return(0);
}
