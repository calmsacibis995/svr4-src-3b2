/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)newoptim:m32/lookup.c	1.5"

#include <string.h>
#include "optab.h"
#include "OpTabTypes.h"

extern struct htabent htab[];
extern struct opent optab[];

unsigned int
lookup(s)
char *s;
{
 unsigned int hash();
 register struct opent *op;
 register struct htabent *hp;

	hp = &htab[ hash(s) ];
	op = hp->op;
	if( op == 0 )
		return( OLOWER );
	else if( strcmp(s, op->oname) == 0 )
		return( op - optab );
	while( hp->next != 0 ){
		hp = hp->next;
		op = hp->op;
		if(strcmp(s, op->oname) == 0)
			return( (unsigned int) (op - optab) );
	}
	return( OLOWER );
}

/************************************
 * Not a perfect hash yet (would be nice).
 * For now this function seems to give a single worst case of
 * 4 strcmp's.
 */
	unsigned int
hash(s)
register char *s;
{
	register hval, j;

	hval = 0; 
	for(j = 0; *s; ++s, ++j){
		hval = (hval << 1) + *s;
		if(j & 01) hval = -hval;
	}
	return(hval % (HTABSZ/2) + (HTABSZ/2));
}

unsigned int
plookup(s)
char *s;
{
 extern struct opent optab[];
 register int c,d;
 register int i;

	/* look up a pseudo op name. s is a name starting with '.';
	   this routine will have to be modified if we insert
	   pseudo ops.
	   The current scheme works because the first and last
	   letters of the pseudo-ops determine the pseudo-op
	   uniquely.
	   For ease of maintenance, it might be better to
	   use binary search, but more string compares would be
	   required in that case.
	 */
	c = *++s;
	d=s[strlen(s)-1]; /* last character */
	switch(c){
	case '2':	i = PS_2BYTE; break;
	case '4':	i = PS_4BYTE; break;
	case 'a':	i = PS_ALIGN; break;
	case 'b':
		switch(d){
		case 's': i= PS_BSS; break;
		case 'e': i= PS_BYTE; break;
		default: return PLOWER;
		}
		break;
	case 'c':	i = PS_COMM; break;
	case 'd':
		switch(d){
		case 'a': i= PS_DATA; break;
		case 'e': i= PS_DOUBLE; break;
		default: return PLOWER;
		}
		break;
	case 'e':	i = PS_EXT; break;
	case 'f':
		switch(d){
		case 'e': i= PS_FILE; break;
		case 't': i= PS_FLOAT; break;
		default: return PLOWER;
		}
		break;
	case 'g':	i = PS_GLOBL; break;
	case 'h':	i = PS_HALF; break;
	case 'i':	i = PS_IDENT; break;
	case 'l':	i = PS_LOCAL; break;
	case 'p':	i = PS_PREVIOUS; break;
	case 's':
		switch(d){
		case 'n': i = PS_SECTION; break;
		case 't': i = PS_SET; break;
		case 'e': i = PS_SIZE; break;
		case 'g': i = PS_STRING; break;
		default: return PLOWER;
		}
		break;
	case 't':
		switch(d){
		case 't': i= PS_TEXT; break;
		case 'e': i= PS_TYPE; break;
		default: return PLOWER;
		}
		break;
	case 'v':	i = PS_VERSION; break;
	case 'w':
		switch(d){
		case 'k': i= PS_WEAK; break;
		case 'd': i= PS_WORD; break;
		default: return PLOWER;
		}
		break;
	case 'z':	i = PS_ZERO; break;
	default:
		return(PLOWER);
	}
	if(strcmp(s, optab[i].oname+1) == 0)
		return(i);
	return(PLOWER);
}
