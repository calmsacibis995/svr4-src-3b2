/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:smtp/src/v9regexp.h	1.2"
#ident "@(#)v9regexp.h	1.2 'attmail mail(1) command'"
/* the structure describing a sub-expression match */
typedef struct {
	char *sp;
	char *ep;
} regsubexp;

/* a compiled regular expression */
typedef char *regexp;

/* the routines */
extern regexp *regcomp();
extern int regexec();
extern void regsub();
