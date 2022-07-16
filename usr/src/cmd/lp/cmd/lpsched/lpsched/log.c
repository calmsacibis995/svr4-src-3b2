/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nlp:cmd/lpsched/log.c	1.1.1.2"

#if	defined(__STDC__)
#include "stdarg.h"
#else
#include "varargs.h"
#endif

#include "lpsched.h"

#if	defined(__STDC__)
static void		log ( char * , va_list );
#else
static void		log();
#endif

/**
 ** open_logfile() - OPEN FILE FOR LOGGING MESSAGE
 ** close_logfile() - CLOSE SAME
 **/

FILE *
#if	defined(__STDC__)
open_logfile (
	char *			name
)
#else
open_logfile (name)
	char			*name;
#endif
{
	register char		*path;

	register FILE		*fp;


#if	defined(MALLOC_3X)
	/*
	 * Don't rely on previously allocated pathnames.
	 */
#endif
	path = makepath(Lp_Logs, name, (char *)0);
	fp = fopen(path, "a");
	free (path);
	return (fp);
}

void
#if	defined(__STDC__)
close_logfile (
	FILE *			fp
)
#else
close_logfile (fp)
	FILE			*fp;
#endif
{
	fclose (fp);
	return;
}

/**
 ** fail() - LOG MESSAGE AND EXIT (ABORT IF DEBUGGING)
 **/

/*VARARGS1*/
void
#if	defined(__STDC__)
fail (
	char *			format,
	...
)
#else
fail (format, va_alist)
	char			*format;
	va_dcl
#endif
{
	va_list			ap;
    
#if	defined(__STDC__)
	va_start (ap, format);
#else
	va_start (ap);
#endif
	log (format, ap);
	va_end (ap);

#if	defined(DEBUG)
	if (debug)
		(void)abort ();
	else
#endif
		exit (1);
	/*NOTREACHED*/
}

/**
 ** note() - LOG MESSAGE
 **/

/*VARARGS1*/
void
#if	defined(__STDC__)
note (
	char *			format,
	...
)
#else
note (format, va_alist)
	char			*format;
	va_dcl
#endif
{
	va_list			ap;

#if	defined(__STDC__)
	va_start (ap, format);
#else
	va_start (ap);
#endif
	log (format, ap);
	va_end (ap);
	return;
}

/**
 ** note() - LOG MESSAGE
 **/

/*VARARGS1*/
void
#if	defined(__STDC__)
schedlog (
	char *			format,
	...
)
#else
schedlog (format, va_alist)
	char			*format;
	va_dcl
#endif
{
	va_list			ap;

#if	defined(DEBUG)
	if (debug) {

#if	defined(__STDC__)
		va_start (ap, format);
#else
		va_start (ap);
#endif
		log (format, ap);
		va_end (ap);

	}
#endif
	return;
}

/**
 ** log() - LOW LEVEL ROUTINE THAT LOGS MESSSAGES
 **/

static void
#if	defined(__STDC__)
log (
	char *			format,
	va_list			ap
)
#else
log (format, ap)
	char			*format;
	va_list			ap;
#endif
{
	int			close_it;

	FILE			*fp;


	if (!am_in_background) {
		fp = stdout;
		close_it = 0;
	} else {
		if (!(fp = open_logfile("lpsched")))
			return;
		close_it = 1;
	}

	if (am_in_background) {
		long			now;

		time (&now);
		fprintf (fp, "%24.24s: ", ctime(&now));
	}
	vfprintf (fp, format, ap);

	if (close_it)
		close_logfile (fp);
	else
		fflush (fp);

	return;
}
