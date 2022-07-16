/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-m32:gen/ldivide.c	1.2"
#ifdef __STDC__
	#pragma weak ldivide = _ldivide
#endif
#include	"synonyms.h"
#include	"sys/types.h"
#include	"sys/dl.h"

dl_t	_dl_zero;

dl_t
ldivide(lop, rop)
dl_t	lop;
dl_t	rop;
{
	register int	cnt;
	dl_t		ans;
	dl_t		tmp;
	dl_t		div;

	if(lsign(lop))
		lop = lsub(_dl_zero, lop);
	if(lsign(rop))
		rop = lsub(_dl_zero, rop);
	
	ans = _dl_zero;
	div = _dl_zero;

	for(cnt = 0 ; cnt < 63 ; cnt++){
		div = lshiftl(div, 1);
		lop = lshiftl(lop, 1);
		if(lsign(lop))
			div.dl_lop |= 1;
		tmp = lsub(div, rop);
		ans = lshiftl(ans, 1);
		if(lsign(tmp) == 0){
			ans.dl_lop |= 1;
			div = tmp;
		}
	}

	return(ans);
}
