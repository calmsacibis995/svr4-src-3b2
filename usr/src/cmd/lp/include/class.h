/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nlp:include/class.h	1.6"

#if	!defined(_LP_CLASS_H)
#define	_LP_CLASS_H

/**
 ** The internal flags seen by the Spooler/Scheduler and anyone who asks.
 **/

#define CS_REJECTED	0x001

/**
 ** The internal copy of a class as seen by the rest of the world:
 **/

/*
 * A (char **) list is an array of string pointers (char *) with
 * a null pointer after the last item.
 */
typedef struct CLASS {
	char   *name;		/* name of class (redundant) */
	char   **members;       /* members of class */
}			CLASS;

/**
 ** Various routines.
 **/

#if	defined(__STDC__)

CLASS		*getclass ( char * );

int		putclass ( char *, CLASS * );
int		delclass ( char * );

void		freeclass ( CLASS * );

#else

CLASS		*getclass();

int		putclass(),
		delclass();

void		freeclass();

#endif

#endif
