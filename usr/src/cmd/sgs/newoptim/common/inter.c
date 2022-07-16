/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)newoptim:common/inter.c	1.3"

#include	<stdio.h>
#include	<malloc.h>
#include	"defs.h"
#include	"OpTabTypes.h"
#include	"RegId.h"
#include	"ANodeTypes.h"
#include	"ANodeDefs.h"
#include	"LoopTypes.h"
#include	"OperndType.h"
#include	"RoundModes.h"
#include	"olddefs.h"
#include	"TNodeTypes.h"
#include	"TNodeDefs.h"
#include	"optab.h"
#include	"optim.h"



	void
addref(an_id)			/* Add text reference to reference list. */
AN_Id an_id;			/* AN_Id of reference. */

{extern int errno;		/* UNINX error number.	*/
 extern void fatal();		/* Handles fatal errors; in common. */
 extern REF *lastref;		/* Pointer to last label reference.	*/
 register REF *r;

 lastref->nextref = (REF *) Malloc(sizeof(REF));
 if(lastref->nextref == NULL)
	fatal("addref: Malloc failed (%d).\n",errno);
 lastref = lastref->nextref;
 r = lastref;
 r->lab = an_id;
 r->nextref = NULL;
 return;
}
	char *
xalloc(n)
register unsigned n;	/* Allocate space.	*/

{extern int errno;		/* UNIX error number.	*/
 extern void fatal();		/* Handles fatal errors; in common. */
 register char *p;

 if ((p = Malloc(n)) == NULL)
	fatal("xalloc: out of space (%d)\n",errno);
 return (p);
}


	void
xfree(p)
char *p;		/* Free up space allocated by xalloc */

{
 Free(p);				/* Return space.	*/
 return;
}
