/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libw:mbftowc.c	1.1"
#include <ctype.h>
#include <stdlib.h>
#include "_wchar.h"
/* returns number of bytes read by *f */
int mbftowc(s, wchar, f, peekc)
char *s;
wchar_t *wchar;
int (*f)();
int *peekc;
{
	register int length;
	register wchar_t intcode;
	register c;
	char *olds = s;
	wchar_t mask;
	
	if((c = (*f)()) < 0)
		return 0;
	*s++ = c;
	if(!multibyte || c < 0200) {
		*wchar = c;
		return(1);
	}
	intcode = 0;
	if (c == SS2) {
		if(!(length = eucw2)) 
			goto lab1;
		mask = P01;
		goto lab2;
	} else if(c == SS3) {
		if(!(length = eucw3)) 
			goto lab1;
		mask = P10;
		goto lab2;
	} 

lab1:
	if(iscntrl(c)) {
		*wchar = c;
		return(1);
	}
	length = eucw1 - 1;
	mask = P11;
	intcode = c & 0177;
lab2:
	if(length < 0)
		return -1;
	
	while(length--) {
		*s++ = c = (*f)();
		if(c < 0200 || iscntrl(c)) {
			if(c >= 0) 
				*peekc = c;
			--s;
			return(-(s - olds));
		}
		intcode = (intcode << 8) | (c & 0177);
	}
	*wchar = intcode | mask;
	return(s - olds);
}	
