/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nlp:lib/forms/wrform.c	1.6"
/* LINTLIBRARY */
# include	<stdio.h>
# include	<string.h>
# include	<errno.h>
# include	"lp.h"
# include	"form.h"

#if	defined(__STDC__)
int			printcmt ( FILE *, char * );
#else	/* defined(__STDC__) */
int			printcmt();
#endif	/* defined(__STDC__) */

int wrform(formp,fp)
FORM *formp;
FILE *fp;
{

    int i;

    for ( i = 0; i < (NFSPEC - 2) ; i++) {
	(void) fprintf(fp,"%s ",fspec[i]);
	switch (i) {
	    case FO_PLEN:   printsdn(fp, formp->plen);
		    	    break;
	    case FO_PWID:   printsdn(fp, formp->pwid);
		    	    break;
	    case FO_NP:     (void) fprintf(fp,"%d\n",formp->np);
		    	    break;
	    case FO_LPI:    printsdn(fp, formp->lpi);
		            break;
	    case FO_CPI:    if (formp->cpi.val == 9999)
				(void) fprintf(fp,"compressed\n");
			    else
				printsdn(fp, formp->cpi);
		    	    break;
	    case FO_CHSET:  (void) fprintf(fp,"%s",formp->chset);
		    	    if (formp->mandatory == 1)
				(void) fprintf(fp,",%s",MANSTR);
		    	    (void) fprintf(fp,"\n");
		    	    break;
	    case FO_RCOLOR: (void) fprintf(fp,"%s\n",formp->rcolor);
		    	    break;
	    }
	}
    if ((formp->comment != (char *)NULL) && fp == stdout)
	(void) printcmt(fp,formp->comment);
    if (formp->conttype != NULL)
    	(void) fprintf(fp,"%s %s\n",fspec[FO_ALIGN],formp->conttype);
    return(0);
}

int wralign(where,fp)
FILE *where;
FILE *fp;
{
    register int n, total = 0;
    char buf[BUFSIZ + 1];

    while ((n = fread(buf,1,BUFSIZ,fp)) != 0) {
	total += n;
	(void) fwrite(buf,1,n,where);
    }
    return (total);
}

void wralert(alertp)
FALERT *alertp;
{
    printalert(stdout, alertp, 0);
    return;
}

int printcmt(where,comment)
FILE *where;
char *comment;
{

    (void) fprintf(where,"%s\n",fspec[FO_CMT]);
    while ( *comment != '\0') {
	if (*comment == '>')
	    comment++;
	while( *comment != '\n') {
	    putc(*comment,where);
	    comment++;
	    }
	putc(*comment,where);
	comment++;
	}
    return(0);
}
