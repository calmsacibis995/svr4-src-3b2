/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:stdio/fputs.c	3.16"
/*LINTLIBRARY*/
/*
 * Ptr args aren't checked for NULL because the program would be a
 * catastrophic mess anyway.  Better to abort than just to return NULL.
 */
#include "synonyms.h"
#include "shlib.h"
#include <stdio.h>
#include "stdiom.h"
#include <string.h>

int
fputs(ptr, iop)
const char *ptr;
register FILE *iop;
{
	register int ndone = 0, n;
	register unsigned char *cptr, *bufend;
	char *p;

	if (_WRTCHK(iop))
		return EOF;
	bufend = _bufend(iop);

	if ((iop->_flag & _IONBF) == 0)  
	{
		for ( ; ; ptr += n) 
		{
			while ((n = bufend - (cptr = iop->_ptr)) <= 0)  
			{
				/* full buf */
				if (_xflsbuf(iop) == EOF)
					return(EOF);
			}
			if ((p = memccpy((char *) cptr, ptr, '\0', n)) != 0)
				n = (p - (char *) cptr) - 1;
			iop->_cnt -= n;
			iop->_ptr += n;
			if (_needsync(iop, bufend))
				_bufsync(iop, bufend);
			ndone += n;
			if (p != 0)  
			{ 
				/* done; flush buffer if line-buffered */
	       			if (iop->_flag & _IOLBF)
	       				if (_xflsbuf(iop) == EOF)
	       					return EOF;
	       			return ndone;
	       		}
		}
	}
	else  
	{
		/* write out to an unbuffered file */
		register unsigned int cnt = strlen(ptr);

		(void)write(iop->_file, ptr, cnt);
		return cnt;
	}
}
