/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:smtp/src/s5date.c	1.3"
#ident "@(#)s5date.c	1.2 'attmail mail(1) command'"
#include <sys/types.h>
#include <time.h>

extern long time();
extern void tzset();
extern char *tzname[];

/*
 *  return an ascii daytime string
 */
extern char *
thedate()
{
	static char date[32];
	char *tp, *dp;
	struct tm *bp;
	long thetime;

	thetime = time((long *)0);
	bp = localtime(&thetime);
	dp = asctime(bp);
	tzset();
	tp = bp->tm_isdst ? tzname[1] : tzname[0];
	sprintf(date, "%.16s %.3s %.4s", dp, tp, dp+20);
	return date;
}
