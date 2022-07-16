/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libns:ind_data.c	1.7.4.1"
#include <stdio.h>
#include <nserve.h>
#include <tiuser.h>
#include <nsaddr.h>
#include "nsdb.h"
#include "stdns.h"
#include "nslog.h"
#include "string.h"
/*******************************************************************
 *
 *	These functions convert data from a machine independent
 *	external format to internal values.
 *
 ******************************************************************/
char	*prplace();
#define c_sizeof(s)	align(L_SIZE + ((s)?strlen(s)+1:1))

/* initialize place in a block	*/
place_p
setplace(bptr,size)
char	*bptr;
int	size;
{
	place_p	retp;

	LOG2(L_TRACE, "(%5d) enter: setplace\n", Logstamp);
	LOG4(L_CONV,"(%5d) setplace(bptr=%x,size=%d)\n",
		Logstamp,bptr,size);
	if ((retp = (place_p) malloc(sizeof(place_t))) == NULL) {
		PLOG3("(%5d) setplace: malloc(%d) failed\n",
			Logstamp,sizeof(place_t));
		LOG2(L_TRACE, "(%5d) leave: setplace\n", Logstamp);
		return(NULL);
	}
	retp->p_start = retp->p_ptr = bptr;
	retp->p_end = bptr + size;

	LOG2(L_TRACE, "(%5d) leave: setplace\n", Logstamp);
	return(retp);
}
/*	realloc pp to a new size	*/
int
explace(pp,size)
place_p	pp;
int	size;
{
	char	*bptr;

	LOG2(L_TRACE, "(%5d) enter: explace\n", Logstamp);
	LOG4(L_CONV,"(%5d) explace(pp=%s,size=%d)\n",
		Logstamp,prplace(pp),size);
	bptr = pp->p_start;

	if (size == 0)	/* increase by DBLKSIZ	*/
		size = (pp->p_end - bptr) + DBLKSIZ;

	if ((pp->p_end - bptr) > size) {
		LOG2(L_TRACE, "(%5d) leave: explace\n", Logstamp);
		return(SUCCESS);	/* don't reduce size	*/
	}

	free(bptr);
	if ((bptr = realloc(bptr,size)) == NULL) {
		PLOG4("(%5d) explace: realloc(%d,%d) failed\n",
			Logstamp,bptr,size);
		LOG2(L_TRACE, "(%5d) leave: explace\n", Logstamp);
		return(FAILURE);
	}
	/* adjust p_ptr and p_end	*/
	pp->p_end = bptr + size;
	pp->p_ptr = bptr + (pp->p_ptr - pp->p_start);
	pp->p_start = bptr;
	LOG4(L_CONV,"(%5d) explace succeeds, returns pp=%s, size=%d\n",
		Logstamp,prplace(pp),size);
	LOG2(L_TRACE, "(%5d) leave: explace\n", Logstamp);
	return(SUCCESS);
}
/*
 * read a string from block
 */
char	*
getstr(pp,str,size)
place_p	pp;
char	*str;
int	size;
{
	int	rsize;	/* real size of string in block	*/
	char	buffer[BUFSIZ];

	LOG2(L_TRACE, "(%5d) enter: getstr\n", Logstamp);
	LOG5(L_CONV,"(%5d) getstr(pp=%s,str=%x,size=%d)\n",
		Logstamp,prplace(pp),(str)?str:"NULL",size);

	if (!bump(pp)) {
		LOG2(L_TRACE, "(%5d) leave: getstr\n", Logstamp);
		return NULL;
	}
	fcanon("c0",pp->p_ptr,buffer);
	pp->p_ptr += c_sizeof(buffer);
	rsize = strlen(buffer);
	if (str == NULL) {
		if ((str=malloc(rsize+1)) == 0) {
			PLOG3("(%5d) getstr: malloc(%d) failed\n",
				Logstamp,rsize+1);
			LOG2(L_TRACE, "(%5d) leave: getstr\n", Logstamp);
			return(NULL);
		}
		size = rsize;
	}
	else if (rsize > size)
		size--;	/* leave room for null	*/

	strncpy(str,buffer,size);
	str[size] = NULL;
	LOG3(L_CONV,"(%5d) getstr returns str=%s\n",
		Logstamp,(str)?str:"NULL");
	LOG2(L_TRACE, "(%5d) leave: getstr\n", Logstamp);
	return(str);
}
/* negative ret could be valid or error, for now assume only positive numbers */
long
getlong(pp)
place_p	pp;
{
	long	ret=0;

	LOG2(L_TRACE, "(%5d) enter: getlong\n", Logstamp);
	LOG3(L_CONV,"(%5d) getlong(pp=%s)\n",
		Logstamp,prplace(pp));

	if (overbyte(pp,L_SIZE)) {
		LOG2(L_TRACE, "(%5d) leave: getlong\n", Logstamp);
		return(-1);
	}

	fcanon("l",pp->p_ptr,(char *) &ret);
	pp->p_ptr += L_SIZE;

	LOG3(L_CONV,"(%5d) getlong returns %ld\n",Logstamp,ret);
	LOG2(L_TRACE, "(%5d) leave: getlong\n", Logstamp);
	return(ret);
}
/*
 *	now the put routines
 */
int
putlong(pp,value)
place_p	pp;
long	value;
{
	int	size;

	LOG2(L_TRACE, "(%5d) enter: putlong\n", Logstamp);
	LOG4(L_CONV,"(%5d) putlong(pp=%s, value=%ld)\n",
		Logstamp,prplace(pp),value);

	if (overbyte(pp,L_SIZE)) {
		LOG2(L_TRACE, "(%5d) leave: putlong\n", Logstamp);
		return(FAILURE);
	}

	bump(pp);
	size = tcanon("l",(char *) &value,pp->p_ptr,0);
	pp->p_ptr += size;
	LOG2(L_TRACE, "(%5d) leave: putlong\n", Logstamp);
	return(SUCCESS);
}
/* put a string into block, return # of bytes copied (including null) */
int
putstr(pp,str)
place_p	pp;
char	*str;
{
	int	i;
	char	*nstr = "";

	LOG2(L_TRACE, "(%5d) enter: putstr\n", Logstamp);
	LOG4(L_CONV,"(%5d) putstr(pp=%s, str=%s)\n",
		Logstamp,prplace(pp),(str)?str:"NULL");

	bump(pp);
	if (!str)
		str = nstr;

	i = c_sizeof(str);
	if (overbyte(pp,i)) {
		LOG3(L_ALL,"(%5d) putstr: overran end size=%d\n",Logstamp,i);
		LOG2(L_TRACE, "(%5d) leave putstr\n", Logstamp);
		return(0);
	}

	i = tcanon("c0",str,pp->p_ptr,0);
	pp->p_ptr += i;
	LOG3(L_CONV,"(%5d) putstr puts %d bytes in block\n",Logstamp,i);

	LOG2(L_TRACE, "(%5d) leave putstr\n", Logstamp);
	return(i);
}
char	*
prplace(pp)
place_p	pp;
{
	static	char	buf[64];

	LOG2(L_TRACE, "(%5d) enter: prplace\n", Logstamp);
	sprintf(buf,"{s=%x, p=%x, e=%x}",pp->p_start,pp->p_ptr,pp->p_end);
	LOG2(L_TRACE, "(%5d) leave: prplace\n", Logstamp);
	return(buf);
}
int
dumpblock(block,size)
char	*block;
int	size;
{
	int	i;
	int	*x;

	LOG2(L_TRACE, "(%5d) enter: dumpblock\n", Logstamp);
	for (i=0; i <= size; i += 4) {
		if (i % 16 == 0)
			printf("\n");
		x = (int *) &block[i];
		printf("%08x ",*x);
	}
	printf("\n");
	LOG2(L_TRACE, "(%5d) leave: dumpblock\n", Logstamp);
}
int
bump(pp)
place_p	pp;
{
	LOG2(L_TRACE, "(%5d) enter: bump\n", Logstamp);
	pp->p_ptr = (char *) align(pp->p_ptr);

	if (pp->p_ptr >= pp->p_end) {
		LOG2(L_TRACE, "(%5d) leave: bump\n", Logstamp);
		return(0);
	} else {
		LOG2(L_TRACE, "(%5d) leave: bump\n", Logstamp);
		return(1);
	}
}
