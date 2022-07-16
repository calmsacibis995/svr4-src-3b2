/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-m32:gen/llog10.c	1.1"
#ifdef __STDC__
	#pragma weak llog10 = _llog10
#endif
#include	"synonyms.h"
#include	<sys/types.h>
#include	<sys/dl.h>

dl_t
llog10(val)
dl_t	val;
{
	dl_t	result;

	result = lzero;
	val    = ldivide(val, lten);

	while(val.dl_hop != 0  ||  val.dl_lop != 0){
		val = ldivide(val, lten);
		result = ladd(result, lone);
	}

	return(result);
}
