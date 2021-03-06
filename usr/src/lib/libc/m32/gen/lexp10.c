/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-m32:gen/lexp10.c	1.1"
#ifdef __STDC__
	#pragma weak lexp10 = _lexp10
#endif
#include	"synonyms.h"
#include	<sys/types.h>
#include	<sys/dl.h>

dl_t
lexp10(exp)
dl_t	exp;
{
	dl_t	result;

	result = lone;

	while(exp.dl_hop != 0  ||  exp.dl_lop != 0){
		result = lmul(result, lten);
		exp    = lsub(exp, lone);
	}

	return(result);
}
