/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)newoptim:m32/Frame.c	1.5"

/************************************************************************/
/*				Frame.c					*/
/*									*/
/*		This file contains the Frame Assignment Utilities. 	*/
/* 	All operations that require knowledge of the implementation 	*/
/*	of the parent node and the frame node reside in this file.	*/
/*									*/
/************************************************************************/

#include	<stdio.h>
#include	"defs.h"
#include	"OpTabTypes.h"
#include	"RegId.h"
#include	"OperndType.h"
#include	"ANodeTypes.h"
#include	"ANodeDefs.h"
#include	"LoopTypes.h"
#include	"RoundModes.h"
#include	"olddefs.h"
#include	"TNodeTypes.h"
#include	"TNodeDefs.h"
#include	"ALNodeType.h"
#include	"FNodeTypes.h"
#include	"FNodeDefs.h"

				/* node for list of frame assigments */
struct framenode 
	{struct framenode *fn_next;	/* list link */
	 FN_Id fn_id;			/* pointer to function called */
	 int fn_nargs;			/* number of arguments in call */
	 int fn_offset;			/* offset of frame */
	};

				/* node for stack of parents */
struct parentnode
	{struct parentnode *pn_next;	/* stack link */
	 boolean pn_nest;		/* true if this entry due to nesting */
	 int pn_offset;			/* offset of frame */
	};

typedef struct framenode *F_Id;
typedef struct parentnode *P_Id;
static struct framenode framehead = 
	{(struct framenode *) NULL,(FN_Id) NULL,0,0};
static F_Id framelast = &framehead;
static P_Id parenttop = (P_Id)NULL;

	 void
DelFrames()			/* delete all frame assignment nodes */

{extern struct framenode framehead;/* head node for frame node list */
 extern void Free();		/* releases memory; interface to free(3) */
 register F_Id fn1,fn2 = NULL;	/* frame node id's */

 for(fn1 = framehead.fn_next; fn1 != (F_Id)NULL; fn1 = fn2)
	{fn2 = fn1->fn_next;
	 Free((char *) fn1);
	}
 framehead.fn_next = (F_Id) NULL;
 framelast = &framehead;
}

	STATIC void
AddFrame(fn_id,nargs,offset)	/* add new frame node to list */
FN_Id fn_id;			/* id of func */
int nargs;			/* number of arguments in call for func */
unsigned long int offset;	/* offset of frame for func */

{
 extern F_Id framelast;		/* id of last node in list */
 extern void fatal();		/* Exits with diagnostic message.	*/
 extern char *Malloc();		/* allocates memory; interface to malloc(3) */
 extern int errno;		/* UNIX error number.	*/

				/* allocate and link new node */
 if((framelast->fn_next = (F_Id)Malloc(sizeof(struct framenode))) == NULL)
	fatal("Frame: memory size exceeded (%d)\n",errno);
 framelast = framelast->fn_next;
 framelast->fn_next = (F_Id)NULL;
				/* fill in data */
 framelast->fn_id = fn_id;
 framelast->fn_nargs = nargs;
 framelast->fn_offset = offset;
}

	boolean
PopParent()			/* remove entry from parent stack */

{extern P_Id parenttop;		/* id of top entry on stack */
 extern void Free();		/* releases memory; interface to free(3) */
 extern void fatal();		/* Exits with diagnostic message.	*/
 P_Id pn;			/* general parent node id */
 boolean nest;			/* return value */

 if(parenttop == (P_Id)NULL)
	fatal("PopParent: no nodes in stack\n");
				/* unlink and free node */
 pn = parenttop;		
 nest = parenttop->pn_nest;
 parenttop = parenttop->pn_next;
 Free(pn);
 return(nest);
}

	void
PushParent(nest,offset)		/* push offset of func onto parent stack */
boolean nest;			/* true if entry due to nesting */
unsigned long offset;		/* offset of function frame */

{extern P_Id parenttop;		/* id of top entry on stack */
 extern char *Malloc();		/* allocates memory; interface to malloc(3) */
 extern void fatal();		/* Exits with diagnostic message.	*/
 extern int errno;		/* UNIX error number.	*/
 P_Id pn;			/* general parent node id */
				/* allocate and link node */
 if((pn = (P_Id) Malloc(sizeof(struct parentnode))) == (P_Id) NULL)
	fatal("PushParent: memory size exceeded (%d)\n",errno);
 pn->pn_next = parenttop;
 parenttop = pn;
				/* insert data */
 parenttop->pn_nest = nest;
 parenttop->pn_offset = offset;
}

	unsigned long int
AssignFrame(fn_id,nargs)
FN_Id fn_id;			/* id of function */
int nargs;			/* number of args in call to function */

{extern void AddFrame();	/* remove a parent node from stack */
 extern struct framenode framehead;/* head node for frame list */
 extern P_Id parenttop;		/* top of parent stack */
 extern FN_Id FuncId;		/* FN_Id of current function */
 F_Id fn;			/* general frame node */
 P_Id pn;			/* general parent node id */
 boolean share = FALSE;		/* true if can share existing frame */

				/* look for an existing frame with same */
				/* function and number of args in call, */
				/* but not in parent list */
 for(fn = framehead.fn_next; fn != (F_Id)NULL; fn = fn->fn_next)
	{if(fn->fn_id != fn_id) continue;
	 if(fn->fn_nargs != nargs) continue;
	 share = TRUE;
	 for(pn = parenttop; pn != (P_Id)NULL; pn = pn->pn_next)
		if(pn->pn_offset == fn->fn_offset) share = FALSE;
	 if(share) break;
	}
 if(share)			/* return frame found */
	{return(fn->fn_offset);
	}
 else				/* create new frame node */
	{AddFrame(fn_id,nargs,GetFnLocSz(FuncId));
	 return(GetFnLocSz(FuncId));
	}
}
