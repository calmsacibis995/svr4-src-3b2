/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kernel:io/ldivide.c	1.1"

#include	"sys/types.h"
#include	"sys/dl.h"

dl_t	dl_zero;

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
		lop = lsub(dl_zero, lop);
	if(lsign(rop))
		rop = lsub(dl_zero, rop);
	
	ans = dl_zero;
	div = dl_zero;

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
