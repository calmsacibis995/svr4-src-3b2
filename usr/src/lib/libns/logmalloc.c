/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libns:logmalloc.c	1.3.1.1"
#include <stdio.h>
#include "nslog.h"
#undef free
#undef malloc
#undef calloc
#undef realloc
char	*malloc();
char	*realloc();
char	*calloc();

void
xfree(p)
char	*p;
{
	LOG3(L_MALLOC,"(%5d) free(0x%x)\n",Logstamp,p);
	fflush(Logfd);
	free(p);
	return;
}
char	*
xmalloc(size)
unsigned size;
{
	char	*ret;
	LOG3(L_MALLOC,"(%5d) malloc(%d)",Logstamp,size);
	fflush(Logfd);
	ret = malloc(size);
	LOG3(L_MALLOC,"(%5d) returns 0x%x\n",Logstamp,ret);
	return(ret);
}
char	*
xrealloc(p,size)
char	*p;
unsigned size;
{
	char	*ret;
	LOG4(L_MALLOC,"(%5d) realloc(0x%x,%d) ",Logstamp,p,size);
	fflush(Logfd);
	ret = realloc(p,size);
	LOG3(L_MALLOC,"(%5d) returns 0x%x\n",Logstamp,ret);
	return(ret);
}
char	*
xcalloc(n,size)
unsigned n,size;
{
	char	*ret;
	LOG4(L_MALLOC,"(%5d) calloc(%d,%d) ",Logstamp,n,size);
	fflush(Logfd);
	ret = calloc(n,size);
	LOG3(L_MALLOC,"(%5d) returns 0x%x\n",Logstamp,ret);
	return(ret);
}
