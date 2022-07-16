/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)boot:boot/boot/strrchr.c	1.1"

/*
 * Return the ptr in sp at which the character c last
 * appears; NULL if not found
*/

char *
strrchr(sp, c)
register char *sp;
register int c;
{
	register char *r = 0;

	do {
		if(*sp == c)
			r = sp;
	} while(*sp++);
	return((char *)r);
}
