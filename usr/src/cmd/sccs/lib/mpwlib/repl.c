/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sccs:lib/mpwlib/repl.c	6.3"
/*
	Replace each occurrence of `old' with `new' in `str'.
	Return `str'.
*/

char *
repl(str,old,new)
char *str;
char old,new;
{
	char	*trnslat();
	return(trnslat(str, &old, &new, str));
}
