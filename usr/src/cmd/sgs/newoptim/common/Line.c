/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)newoptim:common/Line.c	1.2"

#include	<stdio.h>
#include	"defs.h"
#include	"ANodeTypes.h"
#include	"OpTabTypes.h"
#include	"olddefs.h"
#include	"TNodeTypes.h"
#include	"LoopTypes.h"
#include	"OperndType.h"
#include	"RoundModes.h"
#include	"TNodeDefs.h"

/* These routines try to preserve the line number information.
   The criterion for saving or discarding line number information is
   that the state of the machine from a C point of view should be the
   same at any given line both before and after the optimization.
   If it would not be then the line number is deleted.
   The exceptions to the above are that the state of the condition codes 
   and of dead registers is ignored. 
   
	May 1983 */

/* NOTE: All instruction nodes that these routines assume will be tossed
	  have their uniqueids set to IDVAL.  For example, in the merges,
	  the uniqueids of the original instructions are set to IDVAL
	  and then the uniqueid of the resultant instruction is computed.
	  This has the effect of setting the uniqueid of the instruction 
	  not corresponding to the resultant to IDVAL. This fact is used 
	  in some instances where lnmrginst2 is called -- see w2opt.c */

/* delete instruction -- save line number
	if the instruction below has no line number or has a line number
	greater than the present instruction, the present number 
	is transfered down */

	void
ldelin(nodep)
TN_Id  nodep;

{register TN_Id nextnode;	/* TN_Id of next node, if any.	*/
 unsigned long int nextline;	/* Unique id of next text node.	*/
 unsigned long int thisline;	/* Unique id of this text node.	*/

 thisline = GetTxUniqueId(nodep);		/* Get unique id of this node.*/
 if(thisline == IDVAL)				/* If this node has no line */
	return;					/* number, do nothing.	*/

 nextnode = GetTxNextNode(nodep);		/* Get TN_Id of next node. */
 if(nextnode && 
		((nextline = GetTxUniqueId(nextnode)) > thisline  ||
			(nextline == IDVAL)))
	PutTxUniqueId(nextnode,thisline);	/* Put this line's number */
						/* on next node.	*/

 PutTxUniqueId(nodep,IDVAL);		/* Remove line number from this node. */

 return;
}

/* more conservative version of lndelinst that does not overwrite
	line numbers below */
	void
ldelin2(nodep)
TN_Id nodep;
{TN_Id nextnode;		/* TN_Id of next node.	*/
 unsigned long int thisline;	/* Line number of this node.	*/

 thisline = GetTxUniqueId(nodep);		/*Get this node's line number.*/
 if(thisline == IDVAL)				/* If no line-number,	*/
	return;					/* do nothing.	*/

 nextnode = GetTxNextNode(nodep);		/* Get TN_Id of next node. */
 if(nextnode && (GetTxUniqueId(nextnode) == IDVAL))
	PutTxUniqueId(nextnode,thisline);

 PutTxUniqueId(nodep,IDVAL);			/* Remove line-number.	*/

 return;
}


/* exchange instructions
	The line number of the first instruction, if any, is given to the
	second instruction.  The line number of the second instruction is 
	deleted. */
	void
lexchin(nodep1,nodep2)
TN_Id nodep1, nodep2;
{unsigned long int this1;	/* Unique id of nodep1.	*/

 this1 = GetTxUniqueId(nodep1);			/* Get Unique Id of nodep1. */
 if(this1 != IDVAL)
	{PutTxUniqueId(nodep2,this1);
	 PutTxUniqueId(nodep1,IDVAL);
	}
 else
	PutTxUniqueId(nodep2,IDVAL);

 return;
}


/* merge instructions - case 1
	The result instruction is given the line number of the
	top instruction, if it exists, or else it is given the line
	number of the bottom instruction. */
	void
lmrgin1(nodep1,nodep2,resnode)
TN_Id nodep1, nodep2, resnode;
{
 unsigned long int val1;
 unsigned long int val2;

 val1 = GetTxUniqueId(nodep1);
 val2 = GetTxUniqueId(nodep2);

 PutTxUniqueId(nodep1,IDVAL);
 PutTxUniqueId(nodep2,IDVAL);

 if(val1 != IDVAL)
	PutTxUniqueId(resnode,val1);
 else 
	PutTxUniqueId(resnode,val2);

 return;
}


/* instruction merge - case 2
	The resultant instruction is given the line number of the
	top instruction, if it exists, else it is not given a line number */
	void
lmrgin2(nodep1,nodep2,resnode)
TN_Id nodep1, nodep2, resnode;
{unsigned long int val1;

 val1 = GetTxUniqueId(nodep1);

 PutTxUniqueId(nodep1,IDVAL);
 PutTxUniqueId(nodep2,IDVAL);

 PutTxUniqueId(resnode,val1);

 return;
}

	void
lmrgin3(nodep1,nodep2,resnode)
	/* instruction merge - case 3
	The resultant instruction is given the line number of the first
	instruction, if it exists. The instruction below the pair to be 
	merged is given the line number of the second instruction, 
	if it exists */
TN_Id nodep1, nodep2, resnode;
{unsigned long int val1;

 ldelin2(nodep2);

 val1 = GetTxUniqueId(nodep1);

 PutTxUniqueId(nodep1,IDVAL);
 PutTxUniqueId(nodep2,IDVAL);

 PutTxUniqueId(resnode,val1);

 return;
}
