/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)newoptim:common/MallocFree.c	1.4"

#include	<stdio.h>
#include	<malloc.h>
#include	"defs.h"

#ifdef MALLOC

#define MAX	1024

static char *A[MAX];


	void
Free(ptr)			/* Interface to free(3).	*/
char *ptr;

{
 register unsigned int slot;
 extern void fatal();

 for(slot = 0; slot < MAX; slot++)
	{if(A[slot] == NULL)
		continue;
	 if(A[slot] == ptr)
		{A[slot] = NULL;
		 free(ptr);
		 return;
		}
	}
 fatal("Free: attempt to free unallocated 0x%8.8x\n",ptr);

 free(ptr);
 return;
}


	char *
Malloc(size)			/* Interface to malloc(3).	*/
unsigned int size;

{
 register unsigned int slot;
 char *ptr;
 extern void fatal();

 ptr = malloc(size);
 for(slot = 0; slot < MAX; slot++)
	{if(A[slot] == NULL)
		{A[slot] = ptr;
		 return(ptr);
		}
	}
 fatal("Malloc: too many unFree-ed Mallocs.\n");
 return(ptr);
}
#endif /*MALLOC*/
	char *
ExtendCopyBuf(p0,pn,nsize)
char **p0;
char **pn;
unsigned int nsize;
{
 char *b0 = *p0;
 char *bn = *pn;
 unsigned int osize;
 extern void fatal();

 /* input buffer looks like:
  *
  *	-----------------------
  *	|   |   | ... |   |   |
  *	-----------------------
  *       ^                 ^
  *       |                 |
  *	 *p0               *pn == s
  *
  * where the current user pointer, s, is at the end of buffer.
  * after buffer extension, the new buffer looks like:
  *
  *	---------------------------------------------
  *	|   |   | ... |   |   |   |   | ... |   |   |
  *	---------------------------------------------
  *       ^                 ^                     ^
  *       |                 |                     |
  *      *p0                s                    *pn
  *
  * where s is at the same distance from the beginning of the buffer,
  * and the contents of the buffer from *p0 to s is unchanged,
  * and s is returned.
  */

 osize = bn - b0 + 1;
 if(nsize <= osize)
	fatal("ExtendCopyBuf: new size (%d) <= old size (%d)\n",
		nsize, osize);
 b0 = realloc(b0,nsize);
 if(b0 == NULL)
	fatal("ExtendCopyBuf: out of space\n");
 bn = b0 + nsize - 1;
 *p0 = b0;
 *pn = bn;
 return(b0 + osize - 1);
}
	void
ExtendBuf(p0,pn,nsize)
char **p0;
char **pn;
unsigned int nsize;
{
 char *b0 = *p0;
 char *bn = *pn;
 extern void fatal();

 /* input buffer looks like:
  *
  *	-----------------------
  *	|   |   | ... |   |   |
  *	-----------------------
  *       ^                 ^
  *       |                 |
  *	 *p0               *pn
  *
  * this buffer is freed, and a new buffer of size 'nsize' 
  * is gotten.
  */

 if(nsize <= bn - b0 + 1)
	fatal("ExtendBuf: new size (%d) <= old size (%d)\n",
		nsize,bn - b0 + 1);
 Free(b0);
 b0 = Malloc(nsize);
 if(b0 == NULL)
	fatal("ExtendBuf: out of space\n");
 bn = b0 + nsize - 1;
 *p0 = b0;
 *pn = bn;
 return;
}
