/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libw:scrwidth.c	1.2"
#include	"libw.h"
#include	<ctype.h>
#include "_wchar.h"

int scrwidth(c)
wchar_t c;
{
	if(!wisprint(c))
		return(0);
	return(!multibyte || c <= 0177 ? 1 : (c & P11) == P11 ? scrw1 :
		(c & P01) ? scrw2 : scrw3);
}
