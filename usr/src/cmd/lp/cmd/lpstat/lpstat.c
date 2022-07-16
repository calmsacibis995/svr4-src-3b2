/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nlp:cmd/lpstat/lpstat.c	1.9"

#include "stdio.h"
#include "errno.h"
#include "sys/types.h"
#include "signal.h"
#include "stdlib.h"

#include "lp.h"
#include "msgs.h"
#include "printers.h"

#define	WHO_AM_I	I_AM_LPSTAT
#include "oam.h"

#include "lpstat.h"


#ifdef SIGPOLL
void			catch();
#else
int			catch();
#endif

int			exit_rc			= 0,
			inquire_type		= INQ_UNKNOWN,
			scheduler_active	= 0,
			r;		/* Says -r was specified */

char			*alllist[]	= {
	NAME_ALL,
	0
};

/**
 ** main()
 **/

int			main (argc, argv)
	int			argc;
	char			*argv[];
{
	parse (argc, argv);
	done (0);
	/*NOTREACHED*/
}

/**
 ** def()
 **/

void			def ()
{
	char			*name;

	if ((name = getdefault()))
		(void) printf("system default destination: %s\n", name);
	else
		(void) printf("no system default destination\n");

	return;
}

/**
 ** running()
 **/

void			running ()
{
	(void) printf(
		"scheduler is %s\n",
		(scheduler_active ? "running" : "not running")
	);
	return;
}

/**
 ** startup()
 **/

void			startup ()
{
	int			try;


	if (signal(SIGHUP, SIG_IGN) != SIG_IGN)
		(void)signal (SIGHUP, catch);

	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
		(void)signal (SIGINT, catch);

	if (signal(SIGQUIT, SIG_IGN) != SIG_IGN)
		(void)signal (SIGQUIT, catch);

	if (signal(SIGTERM, SIG_IGN) != SIG_IGN)
		(void)signal (SIGTERM, catch);

	for (try = 1; try <= 5; try++) {
		scheduler_active = (mopen() == 0);
		if (scheduler_active || errno != ENOSPC)
			break;
		sleep (3);
	}

	return;
}

/**
 ** catch()
 **/

#ifdef SIGPOLL
void
#else
int
#endif
			catch()
{
	(void)signal (SIGHUP, SIG_IGN);
	(void)signal (SIGINT, SIG_IGN);
	(void)signal (SIGQUIT, SIG_IGN);
	(void)signal (SIGTERM, SIG_IGN);
	done (2);
}

/**
 ** Malloc()
 ** Realloc()
 **/

#if	defined(__STDC__)
typedef void *alloc_type;
#else
typedef char *alloc_type;
#endif

alloc_type
#if	defined(__STDC__)
Malloc (
	size_t			size
)
#else
Malloc (size)
	size_t			size;
#endif
{
	alloc_type		ret	= malloc(size);

	if (!ret) {
		LP_ERRMSG (ERROR, E_LP_MALLOC);
		done (1);
	}
	return (ret);
}

alloc_type
#if	defined(__STDC__)
Realloc (
	void *			ptr,
	size_t			size
)
#else
Realloc (ptr, size)
	char *			ptr;
	size_t			size;
#endif
{
	alloc_type		ret	= realloc(ptr, size);

	if (!ret) {
		LP_ERRMSG (ERROR, E_LP_MALLOC);
		done (1);
	}
	return (ret);
}

alloc_type
#if	defined(__STDC__)
Calloc (
	size_t			nelem,
	size_t			elsize
)
#else
Calloc (nelem, elsize)
	size_t			nelem;
	size_t			elsize;
#endif
{
	alloc_type		ret	= calloc(nelem, elsize);

	if (!ret) {
		LP_ERRMSG (ERROR, E_LP_MALLOC);
		done (1);
	}
	return (ret);
}
