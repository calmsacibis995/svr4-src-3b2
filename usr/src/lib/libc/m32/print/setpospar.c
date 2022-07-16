/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-m32:print/setpospar.c	1.8"
/*LINTLIBRARY*/
/*
 * This file contains a routine needed for the positional parameters feature.
 */

#include "synonyms.h"
#include "shlib.h"
#include <string.h>
#include <ctype.h>

static const char *digits = "01234567890", *skips = "# +-.0123456789hL$";

static lastoffset;	/* offset of first argument not stored in the list
			 * of offsets; this is used by _getparg() as
			 * the base for arguments not in the fast lookup
			 * list (i.e., args to vprintf >= MAXARG) */

void
_setpospar(fmt,offlist,maxlen)
char	*fmt;
unsigned long	*offlist;
int	maxlen;
{
	extern int atoi();
	int n, curargno, size, flags, maxarg, total;

	/*
	* Algorithm	1. set all offsets to zero.
	*		2. walk through fmt putting arg sizes in offlist[].
	*		3. rewalk offlist[] creating cumulative offsets.
	* Assumptions:	Cannot use %*$... to specify variable position.
	*/
	(void)memset((VOID *)offlist, 0, maxlen * sizeof(unsigned long));
	maxarg = -1;
	curargno = 0;
	while ((fmt = strchr(fmt, '%')) != 0)
	{
		fmt++;	/* skip % */
		if (fmt[n = strspn(fmt, digits)] == '$')
		{
			curargno = atoi(fmt) - 1;	/* convert to zero base */
			if (curargno < 0)
				continue;
			fmt += n + 1;
		}
		flags = 0;
	again:;
		fmt += strspn(fmt, skips);
		switch (*fmt++)
		{
		case '%':	/* there is no argument! */
			continue;
		case 'l':
			flags |= 0x1;
			goto again;
		case '*':	/* int argument used for value */
			/* check if there is a positional parameter */
			if (isdigit(*fmt)) {
				int	targno;
				targno = atoi(fmt) - 1;
				fmt += strspn(fmt, digits);
				if (*fmt == '$')
					fmt++; /* skip '$' */
				if (targno >= 0 && targno < maxlen) {
					if (offlist[targno] < sizeof(int))
						offlist[targno]=sizeof(int);
					if (maxarg < targno)
						maxarg = targno;
				}
				goto again;
			}
			flags |= 0x2;
			size = sizeof(int);
			break;
		case 'e':
		case 'E':
		case 'f':
		case 'g':
		case 'G':
			size = sizeof(double);
			break;
		case 's':
			size = sizeof(char *);
			break;
		case 'p':
			size = sizeof(VOID *);
			break;
		case 'n':
			size = (flags & 0x1) ? sizeof(long *) : sizeof(int *);
			break;
		default:
			size = (flags & 0x1) ? sizeof(long) : sizeof(int);
			break;
		}
		if (curargno >= 0 && curargno < maxlen)
		{
			if (offlist[curargno] < size)	/* keep the maximum */
				offlist[curargno] = size;
			if (maxarg < curargno)
				maxarg = curargno;
		}
		curargno++;	/* default to next in list */
		if (flags & 0x2)	/* took care of *, keep going */
		{
			flags ^= 0x2;
			goto again;
		}
	}
	for (total = n = 0; n <= maxarg; n++)	/* accumulate offsets */
	{
		if ((size = offlist[n]) == 0)
			size = sizeof(int);	/* good guess for skipped */
		offlist[n] = total;
		total += size;
	}
	lastoffset = total;
}

#define MAXARG	30

/*
 * This function is used to find the offset of arguments whose
 * position is greater than MAXARG.  This function is slow, so hopefully
 * MAXARG will be big enough so that this function need only be called in
 * unusual circumstances.
 */
int
_getparg(fmt, argno)
char	*fmt;
int	argno;
{
	int i, n, curargno, flags;
	char	*sfmt = fmt;
	int	found = 1;
	int	size = 0;

	i = MAXARG + 1;
	curargno = 1;
	while (found)
	{
		fmt = sfmt;
		found = 0;
		while ((i != argno) && (fmt = strchr(fmt, '%')) != 0)
		{
			fmt++;	/* skip % */
			if (fmt[n = strspn(fmt, digits)] == '$')
			{
				curargno = atoi(fmt);
				if (curargno <= 0)
					continue;
				fmt += n + 1;
			}

			/* find conversion specifier for next argument */
			if (i != curargno)
			{
				curargno++;
				continue;
			} else
				found = 1;
			flags = 0;
		again:;
			fmt += strspn(fmt, skips);
			switch (*fmt++)
			{
			case '%':	/*there is no argument! */
				continue;
			case 'l':
				flags |= 0x1;
				goto again;
			case '*':	/* int argument used for value */
				/* check if there is a positional parameter;
				 * if so, just skip it; its size will be
				 * correctly determined by default */
				if (isdigit(*fmt)) {
					fmt += strspn(fmt, digits);
					if (*fmt == '$')
						fmt++; /* skip '$' */
					goto again;
				}
				flags |= 0x2;
				size += sizeof(int);
				break;
			case 'e':
			case 'E':
			case 'f':
			case 'g':
			case 'G':
				if (flags & 0x1)
					size += sizeof(double);
				else
					size += sizeof(double);
				break;
			case 's':
				size += sizeof(char *);
				break;
			case 'p':
				size += sizeof(VOID *);
				break;
			case 'n':
				if (flags & 0x1)
					size += sizeof(long *);
				else
					size += sizeof(int *);
				break;
			default:
				if (flags & 0x1)
					size += sizeof(long);
				else
					size += sizeof(int);
				break;
			}
			i++;
			curargno++;	/* default to next in list */
			if (flags & 0x2)	/* took care of *, keep going */
			{
				flags ^= 0x2;
				goto again;
			}
		}

		/* missing specifier for parameter, assume parameter is an int */
		if (!found && i != argno) {
			size += sizeof(int);
			i++;
			curargno = i;
			found = 1;
		}
	}
	return(lastoffset + size);
}
