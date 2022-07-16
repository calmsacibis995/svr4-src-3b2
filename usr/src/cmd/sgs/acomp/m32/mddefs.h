/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)acomp:m32/mddefs.h	51.5"
/* mddefs.h */


/* Machine-dependent ANSI C definitions. */


/* These definitions will eventually be in CG's macdefs.h */
#define	SZLDOUBLE	96
#define	ALLDOUBLE	32

/* Select duplicated tests for for-loops and while-loops. */
#define	FOR_LOOP_CODE	LL_DUP
#define	WH_LOOP_CODE	LL_DUP

#ifdef	ELF_OBJ

/* HACK:  register numbers to represent ELF debugging. */
#define	DB_FRAMEPTR	9	/* %fp number */
#define	DB_ARGPTR	10	/* %ap number */

/* Enable #pragma weak.  The two strings are for the 1 and 2
** identifier forms of the pragma.
*/
#define	WEAK_PRAGMA "\t.weak\t%1\n", "\t.weak\t%1\n\t.set\t%1,%2\n"
#endif
