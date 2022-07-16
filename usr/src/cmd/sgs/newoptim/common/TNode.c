/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)newoptim:common/TNode.c	1.10"

/************************************************************************/
/*				TNode.c					*/
/*									*/
/*		This file contains the Text Node utilities. All the	*/
/*	operations that require a knowledge of the implementation of	*/
/*	the text node are meant to reside in this file.			*/
/*									*/
/************************************************************************/

#include	<stdio.h>
#include	<malloc.h>
#include	"defs.h"
#include	"OpTabTypes.h"
#include	"OpTabDefs.h"
#include	"RegId.h"
#include	"ANodeTypes.h"
#include	"ANodeDefs.h"
#include	"LoopTypes.h"
#include	"OperndType.h"
#include	"olddefs.h"
#include	"TNodeTypes.h"
#include	"optab.h"
#include	"optim.h"
#include	"RoundModes.h"

#ifndef MACRO
static TN_Id FirstTNId = {NULL};	/* TN_Id of first text node.	*/
static TN_Id LastTNId = {NULL};		/* TN_Id of last text node.	*/
#else
TN_Id FirstTNId = {NULL};		/* TN_Id of first text node.	*/
TN_Id LastTNId = {NULL};		/* TN_Id of last text node.	*/
#endif /*MACRO*/
	void
DelTxNode(tn_id)		/* Delete a text node.	*/
TN_Id tn_id;			/* Text node to be deleted.	*/


{extern TN_Id FirstTNId;	/* Id of first text node.	*/
 extern TN_Id LastTNId;		/* Id of last text node.	*/
 extern void fatal();		/* Handles fatal errors; in common.	*/
 extern NODE n0;		/* Node before the first.	*/
 extern NODE ntail;		/* Node after the last.	*/
 TN_Id prev;			/* Id of previous text node.	*/
 TN_Id next;			/* Id of next text node.	*/

 if(tn_id == NULL)				/* Did we get one? */
	fatal("DelTxNode: attempting to delete NULL node.\n");
 prev = tn_id->back;				/* Prepare to fix linked list.*/
 next = tn_id->forw;

 if(prev)					/* Fix linked list.	*/
	prev->forw = next;
 else
	{FirstTNId = next;
	 n0.forw = next;
	}
 if(next)
	next->back = prev;
 else
	{LastTNId = prev;
	 ntail.back = prev;
	}

 Free(tn_id);					/* Free the node.	*/

 return;
}
	void
DelTxNodes(start,finish)	/* Delete all text nodes.	*/
TN_Id start;			/* First node to delete.	*/
TN_Id finish;			/* Last node to delete.	*/


{extern TN_Id FirstTNId;	/* Id of first text node.	*/
 extern TN_Id LastTNId;		/* Id of last text node.	*/
 extern NODE n0;
 register TN_Id next_tnid;
 extern NODE ntail;		/* Node after the last node.	*/
 register TN_Id prev_tnid;
 boolean unnamed;

 unnamed = FALSE;		/* Assume named function.	*/
 if(start == (TN_Id) NULL)
	{unnamed = TRUE;
	 start = FirstTNId;
	}
 if(finish == (TN_Id) NULL)
	{unnamed = TRUE;
	 finish = LastTNId;
	}
 while(start)
	{next_tnid = start->forw;
	 prev_tnid = start->back;
	 if(start == &n0)
		{start = next_tnid;
		 continue;
		}
	 if(start == &ntail)
		break;
	 Free(start);
	 prev_tnid->forw = next_tnid;		/* Fix list.	*/
	 next_tnid->back = prev_tnid;
	 start = next_tnid;
	} /* END OF while(start) */

 if(unnamed)
	{FirstTNId = &n0;
	 LastTNId = &ntail;
	 n0.forw = &ntail;
	 ntail.back = &n0;
	}

 return;
}
	void
GetTxDead(tn_id,out,outsize)	/* Get Dead information from text node.	*/
TN_Id tn_id;			/* Text node involved.	*/
unsigned long int out[];	/* Where to put the information.	*/
unsigned int outsize;		/* Amount of information desired.	*/


{extern void fatal();		/* Handles fatal errors; in common.	*/
 extern void fprinst();		/* Prints an instruction.	*/
 register unsigned int item;	/* Item counter.	*/

 if(outsize > (unsigned int)tn_id->DVectors)			/* Is outsize OK? */
	{fprinst(stderr,-2,tn_id);		/* No: show instruction. */
	 fatal("GetTxDead: outsize (%u) too big (%u).\n",
		outsize,tn_id->DVectors);
	}

 for(item = 0; item < outsize; item++)		/* Copy it out.	*/
	out[item] = tn_id->ndead[item];
 return;
}


	void
GetTxLive(tn_id,out,outsize)	/* Get Live information from text node.	*/
TN_Id tn_id;			/* Text node involved.	*/
unsigned long int out[];	/* Where to put the information.	*/
unsigned int outsize;		/* Amount of information desired.	*/


{extern void fatal();		/* Handles fatal errors; in common.	*/
 extern void fprinst();		/* Prints an instruction.	*/
 register unsigned int item;	/* Item counter.	*/

 if(outsize > (unsigned int)tn_id->LVectors)			/* Is outsize OK? */
	{fprinst(stderr,-2,tn_id);		/* No: show instruction. */
	 fatal("GetTxLive: outsize (%u) too big (%u).\n",
		outsize,tn_id->LVectors);
	}

 for(item = 0; item < outsize; item++)		/* Copy it out.	*/
	out[item] = tn_id->nlive[item];
 return;
}
	unsigned int
GetTxLoopDepth(tn_id)		/* Get depth of LOOP node.	*/
TN_Id tn_id;			/* Node whose depth is wanted.	*/

{extern void fatal();		/* Handles fatal errors; in common.	*/
 extern void fprinst();		/* Prints instructions; in Machine Dep.	*/

 if(tn_id->op != LOOP)				/* If not a LOOP node, */
	{fprinst(stderr,-2,tn_id);
	 fatal("GetTxLoopDepth: above line not a LOOP node.\n");
	}

 return((unsigned int) tn_id->addrs[1]);	/* Get from node.	*/
}


	boolean
GetTxLoopFlag(tn_id)		/* Get loop nest OK flag.	*/
TN_Id tn_id;			/* Node whose next OK flag is wanted.	*/

{extern void fatal();		/* Handles fatal errors; in common.	*/
 extern void fprinst();		/* Prints instructions; in Machine Dep.	*/

 if(tn_id->op != LOOP)				/* If not a LOOP node, */
	{fprinst(stderr,-2,tn_id);		/* complain.	*/
	 fatal("GetTxLoopFlag: above line not a LOOP node.\n");
	}

 if((unsigned int)tn_id->addrs[3] == 1)
	return(TRUE);
 else
	return(FALSE);
}


	unsigned int
GetTxLoopSerial(tn_id)		/* Get LOOP node serial number.	*/
TN_Id tn_id;			/* Node whose LOOP serial number is wanted. */

{extern void fatal();		/* Handles fatal errors; in common.	*/
 extern void fprinst();		/* Prints instructions; in Machine Dep.	*/

 if(tn_id->op != LOOP)				/* If not a LOOP node, */
	{fprinst(stderr,-2,tn_id);		/* complain.	*/
	 fatal("GetTxLoopSerial: above line not a LOOP node.\n");
	}

 return((unsigned int) tn_id->addrs[2]);	/* Get it from node.	*/
}
	LoopType
GetTxLoopType(tn_id)		/* Get Loop Type from text node.	*/
TN_Id tn_id;			/* Id from which to get loop type.	*/

{extern void fatal();		/* Handles fatal errors; in common.	*/
 extern void fprinst();		/* Prints instructions; in Machine Dep.	*/

 if(tn_id->op != LOOP)				/* If not a LOOP node, */
	{fprinst(stderr,-2,tn_id);
	 fatal("GetTxLoopType: above line not a LOOP node.\n");
	}

 return((LoopType) tn_id->addrs[0]);
}

	TN_Id
GetTxNextNode(tn_id)		/* Get next text node.	*/
TN_Id tn_id;			/* Where to start.	*/


{extern TN_Id FirstTNId;	/* Identifier of first text node.	*/
 extern NODE ntail;		/* Node after the last one.	*/

 if(tn_id == NULL)				/* If we start at beginning, */
	tn_id = FirstTNId;			/* start with the first one. */
 if(tn_id->forw == &ntail)
	return((TN_Id) NULL);
 return(tn_id->forw);				/* Otherwise, return the next.*/
}

	unsigned short int
GetTxOpCodeX(tn_id)		/*Get the operation code index of a text node.*/
TN_Id tn_id;			/* TN_Id of text node.	*/


{
 return(tn_id->op);				/* Get value from text node. */
}
	AN_Id
GetTxOperandAd(tn_id,operand)	/* Get operand address identifier.	*/
TN_Id tn_id;			/* Text node identifier.	*/
unsigned int operand;		/* Operand index.	*/


{extern unsigned int Max_Ops;	/* Maximum operands in an instruction.	*/
 extern void fatal();		/* Handles fatal errors; in common.	*/

 if(operand >= Max_Ops)				/* If operand too big, */
	fatal("GetTxOperandAd: invalid operand (%u).\n",operand);

 return(tn_id->addrs[operand]);			/* Get value from text node. */
}


	unsigned char
GetTxOperandFlags(tn_id,operand)	/* Get operand flags.	*/
TN_Id tn_id;			/* Text node identifier.	*/
unsigned int operand;		/* Operand index.	*/


{extern unsigned int Max_Ops;	/* Maximum operands in an instruction.	*/
 extern void fatal();		/* Handles fatal errors; in common.	*/

 if(operand >= Max_Ops)				/* If operand too big, */
	fatal("GetTxOperandFlags: invalid operand (%u).\n",operand);

 return(tn_id->flags[operand]);	/* Get from text node.*/
}


	AN_Mode
GetTxOperandMode(tn_id,operand)	/* Gets mode of an operand.	*/
TN_Id tn_id;			/* TN_Id of node whose operand mode is wanted.*/
unsigned int operand;		/* Which operand's mode is wanted.	*/

{extern unsigned int Max_Ops;	/* Maximum operands in an instruction.	*/
 extern void fatal();		/* Handles fatal errors; in common.	*/

 if(operand >= Max_Ops)				/* If operand too big, */
	fatal("GetTxOperandMode: invalid operand (%u).\n",operand);

 return(GetAdMode(tn_id->addrs[operand]));
}


	OperandType
GetTxOperandType(tn_id,operand)	/* Get operand type.	*/
TN_Id tn_id;			/* Text node identifier.	*/
unsigned int operand;		/* Operand index.	*/


{extern unsigned int Max_Ops;	/* Maximum operands in an instruction.	*/
 extern void fatal();		/* Handles fatal errors; in common.	*/

 if(operand >= Max_Ops)				/* If operand too big, */
	fatal("GetTxOperandType: invalid operand (%u).\n",operand);

 return((OperandType) tn_id->types[operand]);	/* Get from text node.*/
}

#ifndef MACRO
	TN_Id
GetTxPrevNode(tn_id)		/* Get previous text node.	*/
TN_Id tn_id;			/* Where to start.	*/


{extern TN_Id LastTNId;		/* Identifier of last text node.	*/
 extern NODE n0;		/* Node before the first one.	*/

 if(tn_id == NULL)				/* If we start at end, */
	tn_id = LastTNId;			/* start with the last one. */
 if(tn_id->back == &n0)
	return((TN_Id) NULL);
 return(tn_id->back);				/* Otherwise, return the prev.*/
}
#endif

	RoundMode
GetTxRMode(tn_id)		/* Get MIS rounding mode of tn_id).	*/
TN_Id tn_id;			/* TN_Id of node whose rounding mode wanted. */

{
 return((RoundMode) tn_id->RoundMode);
}


	unsigned long int
GetTxUniqueId(tn_id)		/* Get unique identifier of a text node. */
TN_Id tn_id;			/* TN_Id of text node.	*/


{
 return(tn_id->uniqueid);		/* Get it from text node.	*/
}


	boolean
IsTxAnyCall(tn_id)		/* TRUE if this is any kind of call.	*/
TN_Id tn_id;			/* TN_Id of node to be tested.	*/

{
 if(tn_id->op == CALL)
	return(TRUE);
 if(tn_id->op == ICALL)
	return(TRUE);
 if(tn_id->op == IJSB)
	return(TRUE);
 if(tn_id->op == JSB)
	return(TRUE);
 return(FALSE);
}
	boolean
IsTxAuditerr(tn_id)		/* Is Text Node's Auditerr flag set? */
TN_Id tn_id;			/* Text node to test.	*/

{
 if(tn_id->auditerr == 1)
	return(TRUE);
 else
	return(FALSE);
}

#ifndef MACRO
	boolean
IsTxBlackBox(tn_id)		/* Is Text Node's BlackBox flag set? */
TN_Id tn_id;			/* Text node to test.	*/

{
 if(tn_id->blackbox == 1)
	return(TRUE);
 else
	return(FALSE);
}
#endif

#ifndef MACRO
	boolean
IsTxAux(tn_id)			/* TRUE if tn_id is an AUX node.	*/
TN_Id tn_id;

{
 if(tn_id->op <= AUPPER)
	return(TRUE);
 return(FALSE);
}
#endif

#ifndef MACRO
	boolean			/* TRUE if any branch.	*/
IsTxBr(tn_id)
TN_Id tn_id;

{extern struct opent optab[];	/* The operation code table.	*/

 if(optab[tn_id->op].oflags & (CBR | UNCBR))
	return(TRUE);
 return(FALSE);
}
#endif
	boolean			/* TRUE if conditional branch.	*/
IsTxCBr(tn_id)
TN_Id tn_id;

{extern struct opent optab[];	/* The operation code table.	*/

 if(optab[tn_id->op].oflags & CBR)
	return(TRUE);
 return(FALSE);
}


	boolean
IsTxCPUOpc(tn_id)		/* TRUE if tn_id has a CPU Opcode.	*/
register TN_Id tn_id;

{extern struct opent optab[];	/* The operation code table.	*/

 if(optab[tn_id->op].oflags & CPUOPC)
	return(TRUE);
 return(FALSE);
}

	boolean
IsTxGenOpc(tn_id)		/* TRUE if tn_id has a Generic Opcode.	*/
register TN_Id tn_id;

{
 if(tn_id->op < GLOWER)
	return(FALSE);
 if(tn_id->op > GUPPER)
	return(FALSE);
 return(TRUE);
}


	boolean
IsTxHL(tn_id)			/* TRUE if tn_id is a hard label.	*/
TN_Id tn_id;

{
 if(tn_id->op == HLABEL)
	return(TRUE);
 return(FALSE);
}

#ifndef MACRO
	boolean
IsTxLabel(tn_id)		/* TRUE if tn_id is any label.	*/
register TN_Id tn_id;

{
 if(tn_id == (TN_Id) NULL)			/* If no node at all,	*/
	return(FALSE);				/* it is not a label.	*/
 if(tn_id->op == LABEL)
	return(TRUE);
 if(tn_id->op == HLABEL)
	return(TRUE);
 return(FALSE);
}
#endif
	boolean
IsTxMIS(tn_id)			/* TRUE if tn_id is an MIS instruction.	*/
TN_Id tn_id;

{extern struct opent optab[];	/* The operation code table.	*/

 if(optab[tn_id->op].oflags & MISOPC)
	return(TRUE);
 return(FALSE);
}


#ifndef MACRO
	boolean
IsTxOperandVol(tn_id, opn)	/* Is operand opn volatile? */
TN_Id tn_id;
unsigned int opn;

{extern unsigned int Max_Ops;
 extern void fatal();

 if(opn >= Max_Ops)
	fatal("IsTxOperandVol: operand illegal (%u).\n", opn);

 if((tn_id->flags[opn]) & VOLATILE)
	return TRUE;
 return FALSE;
}
#endif


	boolean
IsTxProtected(tn_id)		/* Is Text Node's Protected flag set? */
TN_Id tn_id;			/* Text node to test.	*/

{
 if(tn_id->protected == 1)
	return(TRUE);
 else
	return(FALSE);
}


	boolean
IsTxRet(tn_id)			/* TRUE if tn_id is a return.	*/
TN_Id tn_id;

{extern struct opent optab[];	/* The operation code table.	*/

 if(optab[tn_id->op].oflags & PRET)
	return(TRUE);
 return(FALSE);
}


	boolean
IsTxRev(tn_id)			/* TRUE if reversable instruction.	*/
TN_Id tn_id;			/* Instruction to be tested.	*/

{extern struct opent optab[];	/* The operation code table.	*/

 if(optab[tn_id->op].oflags & REV)
	return(TRUE);
 return(FALSE);
}


	void
RevTxBr(tn_id)			/* Reverse branch instruction. */
TN_Id tn_id;			/* TN_Id of instruction to be reversed. */

{extern void fatal();		/* Handles fatal errors.	*/
 extern struct opent optab[];	/* The operation code table.	*/

 if((optab[tn_id->op].oflags & REV) == 0)	/*Can instruction be reversed?*/
	fatal("RevTxBr: non-reversable instruction.\n");	/* No.	*/
 if((optab[tn_id->op].oflags & (CBR | UNCBR)) == 0)	/* Is it a branch? */
	fatal("RevTxBr: not a branch.\n");		/* No.	*/

 tn_id->op = optab[tn_id->op].oopcode;		/* Yes: reverse opcode.	*/
 return;
}


	boolean
IsTxSPI(tn_id)			/* TRUE iff text node is stack pointer increm.*/
TN_Id tn_id;

{extern void fatal();		/* Handles fatal errors.	*/

 if(tn_id == (TN_Id) NULL)
	fatal("IsTxSPI: null text node identifier.\n");

 if(tn_id->StackPtrInc == 1)
	return(TRUE);
 else
	return(FALSE);
}

	boolean
IsTxSame(p,q)			/* TRUE iff nodes are the same.	*/
TN_Id p;			/* TN_Id of one of the nodes.	*/
TN_Id q;			/* TN_Id of the other of the nodes.	*/

{extern unsigned int Max_Ops;	/* Maximum operands in an instruction.	*/
 register unsigned int operand;	/* Operand counter.	*/

 if(p->op != q->op)				/* Are op-codes the same? */
	return(FALSE);				/* No.	*/

 for(operand = 0; operand < Max_Ops; operand++)
	{if(GetTxOperandAd(p,operand) != GetTxOperandAd(q,operand))
		return(FALSE);			/* Operand addresses differ. */
	 if(GetTxOperandType(p,operand) != GetTxOperandType(q,operand))
		return(FALSE);			/* Operand types differ. */
	}
 return(TRUE);
}


	boolean
IsTxSameOps(p,q)		/* TRUE iff operands are the same.	*/
TN_Id p;			/* TN_Id of one of the operands.	*/
TN_Id q;			/* TN_Id of the other of the operands.	*/

{extern unsigned int Max_Ops;	/* Maximum operands in an instruction.	*/
 register unsigned int operand;	/* Operand counter.	*/
 extern struct opent optab[];	/* The operation code table.	*/

 if((p->op < AUPPER) &&				/* If these are aux nodes, */
		(q->op < AUPPER))
	return(TRUE);
 if(p->op < AUPPER)
	{for(operand = 0; operand < Max_Ops; operand++)
		if(optab[p->op].otype[operand] != TNONE)
			return(FALSE);
	 return(TRUE);
	}
 if(q->op < AUPPER)
	{for(operand = 0; operand < Max_Ops; operand++)
		if(optab[q->op].otype[operand] != TNONE)
			return(FALSE);
	 return(TRUE);
	}
 for(operand = 0; operand < Max_Ops; operand++)
	{if(GetTxOperandAd(p,operand) != GetTxOperandAd(q,operand))
		return(FALSE);			/* If any mismatch,	*/
	 if(GetTxOperandType(p,operand) != GetTxOperandType(q,operand))
		return(FALSE);			/* not the same.	*/
	}
 return(TRUE);					/* Everything matches.	*/
}
	boolean
IsTxUncBr(tn_id)		/* TRUE if tn_id is an unconditional branch. */
TN_Id  tn_id;

{extern struct opent optab[];	/* The operation code table.	*/

 if(optab[tn_id->op].oflags & UNCBR)
	return(TRUE);
 return(FALSE);
}
	TN_Id
MakeTxNodeAfter(tn_id,OpCode)	/* Make a text node and insert it after spec. */
TN_Id tn_id;			/* TN_Id of node this one goes after.	*/
unsigned int OpCode;	/* Operation code index for this instruction. */


{extern TN_Id FirstTNId;	/* TN_Id of first text node, if any.	*/
 extern TN_Id LastTNId;		/* TN_Id of last text node, if any.	*/
 extern unsigned int Max_Ops;	/* Maximum Operands in an instruction.	*/
 register OperandType dtype;	/* Default operand type.	*/
 extern int errno;		/* UNIX error number.	*/
 extern void fatal();		/* Handles fatal errors; in common.	*/
 extern AN_Id i0;		/* AN_Id of immediate 0.	*/
 register unsigned int operand;	/* Operand counter for initialization.	*/
 register TN_Id newnode;	/* TN_Id of node we are creating.	*/
 TN_Id next;			/* Pointer from current node.	*/
 extern NODE ntail;		/* Node after the last.	*/

 newnode = (TN_Id) Malloc(sizeof(struct node));	/* Get memory for it.	*/
 if(newnode == NULL)				/* Did we get it?	*/
	fatal("MakeTxNodeAfter: Malloc failed (%d).\n",errno);	/* No.	*/

 if(!tn_id)					/* Insert new node in chain. */
	tn_id = FirstTNId;			/* If none, put before first.*/
 next = tn_id->forw;
 tn_id->forw = newnode;
 newnode->forw = next;
 newnode->back = tn_id;
 if(next)
	next->back = newnode;
 else
	{LastTNId = newnode;
	 ntail.back = newnode;
	}

 newnode->auditerr = 0;				/* Initialize new node.	*/
 newnode->blackbox = 0;				/* Not blackbox. */
 newnode->op = (unsigned short)OpCode;		/* Insert the OpCode. */
 newnode->mark = 0;
 newnode->protected = 0;
 newnode->RoundMode = (unsigned) Default;
 newnode->StackPtrInc = 0;
 newnode->DVectors = 0;				/* No Dead data yet.	*/
 newnode->LVectors = 0;				/* No Live data yet.	*/
 newnode->uniqueid = IDVAL;			/* Initialize unique id. */
 if(IsOpGeneric(OpCode))				/* Generic op-codes get a */
	dtype = Tword;				/* default type of Tword. */
 else						/* The rest get a default */
	dtype = Tnone;				/* type of Tnone.	*/
 for(operand = 0; operand < Max_Ops; operand++)
	{newnode->addrs[operand] = i0;	/* Unused have address of immediate 0.*/
	 newnode->types[operand] = (unsigned char) dtype;
	 newnode->flags[operand] = 0;
	}

 return(newnode);				/*Return the new node's TN_Id.*/
}
	TN_Id
MakeTxNodeBefore(tn_id,OpCode)	/* Make a text node and insert it before spec.*/
TN_Id tn_id;			/* TN_Id of node this one goes after. */
unsigned int OpCode;	/* Operation code index for this instruction. */

{extern TN_Id FirstTNId;	/* TN_Id of first text node, if any. */
 extern TN_Id LastTNId;		/* TN_Id of last text node, if any. */
 extern unsigned int Max_Ops;	/* Maximum Operands in an instruction.	*/
 register OperandType dtype;	/* Default operand type.	*/
 extern int errno;		/* UNIX error number.	*/
 extern void fatal();		/* Handles fatal errors; in common. */
 extern  AN_Id i0;		/* AN_Id of immediate 0. */
 register unsigned int operand;	/* Operand counter for initialization. */
 extern NODE n0;		/* Node before the first.	*/
 register TN_Id newnode;	/* TN_Id of node we are creating. */
 TN_Id prev;			/* Pointer from current node. */

 newnode = (TN_Id) Malloc(sizeof(struct node));	/* Get memory for it. */
 if(newnode == NULL)				/* Did we get it? */
	fatal("MakeTxNodeBefore: Malloc failed (%d).\n",errno);	/* No. */

 if(!tn_id)					/* Insert new node in chain. */
	tn_id = LastTNId;			/* If none, put after last. */
 prev = tn_id->back;
 tn_id->back = newnode;
 newnode->forw = tn_id;
 newnode->back = prev;
 if(prev)
	prev->forw = newnode;
 else
	{FirstTNId = newnode;
	 n0.forw = newnode;
	}

 newnode->auditerr = 0;				/* Initialize new node.	*/
 newnode->blackbox = 0;				/* Not blackbox. */
 newnode->op = (unsigned short)OpCode;		/* Insert the OpCode. */
 newnode->mark = 0;
 newnode->protected = 0;
 newnode->RoundMode = (unsigned) Default;
 newnode->DVectors = 0;				/* No Dead data yet.	*/
 newnode->LVectors = 0;				/* No Live data yet.	*/
 newnode->uniqueid = IDVAL;			/* Initialize unique id. */
 if(IsOpGeneric(OpCode))				/* Generic op-codes get a */
	dtype = Tword;				/* default type of Tword. */
 else						/* The others get a default */
	dtype = Tnone;				/* type of Tnone.	*/
 for(operand = 0; operand < Max_Ops; operand++)
	{newnode->addrs[operand] = i0;	/* Unused have address of immediate 0.*/
	 newnode->types[operand] = (unsigned char) dtype;
	 newnode->flags[operand] = 0;
	}

 return(newnode);				/*Return the new node's TN_Id.*/
}
	TN_Id
MoveTxNodeAfter(s_id,d_id)	/* Move a text node after another. */
TN_Id s_id;			/* TN_Id of node to be moved. */
TN_Id d_id;			/* TN_Id of node after which s_id is to go. */


{extern TN_Id FirstTNId;	/* TN_Id of first text node. */
 extern TN_Id LastTNId;		/* TN_Id of last text node. */
 TN_Id AfterD;
 TN_Id AfterS;
 TN_Id BeforeS;
 extern void fatal();		/* Handles fatal errors; in common. */
 extern NODE n0;		/* Node before the first.	*/
 extern NODE ntail;		/* Node after the last.	*/

 if(s_id == NULL)				/* Validate input. */
	fatal("MoveTxNodeAfter: missing source.\n");
 if(d_id == NULL)				/* Validate input. */
	fatal("MoveTxNodeAfter: missing destination.\n");

 AfterD = d_id->forw;
 AfterS = s_id->forw;
 BeforeS = s_id->back;

 if(BeforeS)			/* Remove source node from chain. */
	BeforeS->forw = AfterS;
 else
	{FirstTNId = AfterS;
	 n0.forw = AfterS;
	}
 if(AfterS)
	AfterS->back = BeforeS;
 else
	{LastTNId = BeforeS;
	 ntail.back = BeforeS;
	}

 if(AfterD)			/* Insert source node in new location. */
	AfterD->back = s_id;
 else
	{LastTNId = s_id;
	 ntail.back = s_id;
	}
 d_id->forw = s_id;
 s_id->back = d_id;
 s_id->forw = AfterD;

 return(s_id);
}
	TN_Id
MoveTxNodeBefore(s_id,d_id)	/* Move a text node before another. */
TN_Id s_id;			/* TN_Id of node to be moved. */
TN_Id d_id;			/* TN_Id of node before which s_id is to go. */


{extern TN_Id FirstTNId;	/* TN_Id of first text node. */
 extern TN_Id LastTNId;		/* TN_Id of last text node. */
 TN_Id BeforeD;
 TN_Id AfterS;
 TN_Id BeforeS;
 extern void fatal();		/* Handles fatal errors; in common. */
 extern NODE n0;		/* Node before the first.	*/
 extern NODE ntail;		/* Node after the last.	*/

 if(s_id == NULL)				/* Validate input. */
	fatal("MoveTxNodeBefore: missing source.\n");
 if(d_id == NULL)				/* Validate input. */
	fatal("MoveTxNodeBefore: missing destination.\n");

 BeforeD = d_id->back;
 AfterS = s_id->forw;
 BeforeS = s_id->back;

 if(BeforeS)			/* Remove source node from chain. */
	BeforeS->forw = AfterS;
 else
	{FirstTNId = AfterS;
	 n0.forw = AfterS;
	}
 if(AfterS)
	AfterS->back = BeforeS;
 else
	{LastTNId = BeforeS;
	 ntail.back = BeforeS;
	}

 if(BeforeD)			/* Insert source node in new location. */
	BeforeD->forw = s_id;
 else
	{FirstTNId = s_id;
	 n0.forw = s_id;
	}
 d_id->back = s_id;
 s_id->forw = d_id;
 s_id->back = BeforeD;

 return(s_id);
}
	TN_Id
ReadTxNodeAfter(tn_id,stream)	/* Read a text node and insert it after spec. */
TN_Id tn_id;			/* TN_Id of node this one goes after.	*/
FILE *stream;			/* Stream pointer of stream to read.	*/

{extern TN_Id FirstTNId;	/* TN_Id of first text node, if any.	*/
 extern TN_Id LastTNId;		/* TN_Id of last text node, if any.	*/
 extern int errno;		/* UNIX error number.	*/
 extern void fatal();		/* Handles fatal errors; in common.	*/
 register TN_Id newnode;	/* TN_Id of node we are creating.	*/
 TN_Id next;			/* Pointer from current node.	*/
 extern NODE ntail;		/* Node after the last.	*/

 newnode = (TN_Id) Malloc(sizeof(struct node));	/* Get memory for it.	*/
 if(newnode == NULL)				/* Did we get it?	*/
	fatal("ReadTxNodeAfter: Malloc failed (%d).\n",errno);	/* No.	*/

						/* Read data into node.	*/
 if(fread(newnode,sizeof(struct node),1,stream) == 0)
	{if(feof(stream))			/* Probably end-of-file. */
		return((TN_Id) NULL);		/* Yes: e.o.f.	*/
	 fatal("ReadTxNodeAfter: couldn't read node (%d).\n",errno);
	}

 if(!tn_id)					/* Insert new node in chain. */
	tn_id = FirstTNId;			/* If none, put before first.*/
 next = tn_id->forw;
 tn_id->forw = newnode;
 newnode->forw = next;
 newnode->back = tn_id;
 if(next)
	next->back = newnode;
 else
	{LastTNId = newnode;
	 ntail.back = newnode;
	}

 return(newnode);				/*Return the new node's TN_Id.*/
}
	TN_Id
ReadTxNodeBefore(tn_id,stream)	/* Read a text node and insert it before spec.*/
TN_Id tn_id;			/* TN_Id of node this one goes before.	*/
FILE *stream;			/* Stream pointer of stream to read.	*/

{extern TN_Id FirstTNId;	/* TN_Id of first text node, if any.	*/
 extern TN_Id LastTNId;		/* TN_Id of last text node, if any.	*/
 extern int errno;		/* UNIX error number.	*/
 extern void fatal();		/* Handles fatal errors; in common.	*/
 register TN_Id newnode;	/* TN_Id of node we are creating.	*/
 TN_Id prev;			/* Pointer from current node.	*/
 extern NODE n0;		/* Node before the first.	*/

 newnode = (TN_Id) Malloc(sizeof(struct node));	/* Get memory for it.	*/
 if(newnode == NULL)				/* Did we get it?	*/
	fatal("ReadTxNodeBefore: Malloc failed (%d).\n",errno);	/* No.	*/

						/* Read data into node.	*/
 if(fread(newnode,sizeof(struct node),1,stream) == 0)
	{if(feof(stream))			/* Probably end-of-file. */
		return((TN_Id) NULL);		/* Yes: e.o.f.	*/
	 fatal("ReadTxNodeBefore: couldn't read node (%d).\n",errno);
	}

 if(!tn_id)					/* Insert new node in chain. */
	tn_id = LastTNId;			/* If none, put after last.*/
 prev = tn_id->back;
 tn_id->back = newnode;
 newnode->forw = tn_id;
 newnode->back = prev;
 if(prev)
	prev->forw = newnode;
 else
	{FirstTNId = newnode;
	 n0.forw = newnode;
	}

 return(newnode);				/*Return the new node's TN_Id.*/
}
	void
PutTxAuditerr(tn_id,status)	/* Set Text Node's Auditerr flag. */
TN_Id tn_id;			/* TN_Id of text node. */
boolean status;			/* How it should be set. */

{
 if(status)
	tn_id->auditerr = 1;
 else
	tn_id->auditerr = 0;
 return;

}
	void
PutTxBlackBox(tn_id,status)	/* Set Text Node's BlackBox flag. */
TN_Id tn_id;			/* TN_Id of text node. */
boolean status;			/* How it should be set. */

{
 if(status)
	tn_id->blackbox = 1;
 else
	tn_id->blackbox = 0;
 return;
}


	void
PutTxDead(tn_id,in,insize)	/* Put Dead information into text node. */
TN_Id tn_id;			/* Text node involved. */
unsigned long int in[];		/* Where to get the information. */
unsigned int insize;		/* Amount of information desired. */


{extern void fatal();		/* Handles fatal errors; in common. */
 register unsigned int item;	/* Item counter. */

 if(insize > NVECTORS)				/* Is insize OK? */
	fatal("PutTxDead: insize (%u) too large (%u).\n",
		insize,NVECTORS);
 tn_id->DVectors = insize;			/* Record amount present. */
 for(item = 0; item < insize; item++)		/* Copy it in. */
	tn_id->ndead[item] = in[item];
 return;
}
	void
PutTxLive(tn_id,in,insize)	/* Put Live information into text node. */
TN_Id tn_id;			/* Text node involved. */
unsigned long int in[];		/* Where to get the information. */
unsigned int insize;		/* Amount of information desired. */


{extern void fatal();		/* Handles fatal errors; in common. */
 register unsigned int item;	/* Item counter. */

 if((insize > NVECTORS) || (insize == 0))	/* Is insize OK? */
	fatal("PutTxLive: insize (%u) incorrect (%u).\n",
		insize,NVECTORS);
 tn_id->LVectors = insize;			/* Record amount present. */
 for(item = 0; item < insize; item++)		/* Copy it in. */
	tn_id->nlive[item] = in[item];
 return;
}
	void
PutTxLoopDepth(tn_id,depth)	/* Put depth in LOOP node. */
TN_Id tn_id;			/* Node in which to insert depth. */
unsigned int depth;		/* Depth of loop node. */

{extern void fatal();		/* Handles fatal errors; in common. */
 extern void fprinst();		/* Prints instructions; in Mach. Dep. */

 if(tn_id->op != LOOP)				/* If not a LOOP node, */
	{fprinst(stderr,-2,tn_id);		/* complain. */
	 fatal("PutTxLoopDepth: above line not a LOOP node.\n");
	}

 tn_id->addrs[1] = (AN_Id) depth;		/* Insert in node. */

 return;
}


	void
PutTxLoopSerial(tn_id,serial)	/* Put serial number in LOOP node. */
TN_Id tn_id;			/* Node in which to insert depth. */
unsigned int serial;		/* Serial number of loop node. */

{extern void fatal();		/* Handles fatal errors; in common. */
 extern void fprinst();		/* Prints instructions; in Mach. Dep. */

 if(tn_id->op != LOOP)				/* If not a LOOP node, */
	{fprinst(stderr,-2,tn_id);		/* complain. */
	 fatal("PutTxLoopSerial: above line not a LOOP node.\n");
	}
						/* Make it fit in place. */
 tn_id->addrs[2] = (AN_Id) serial;	/* Insert in node. */

 return;
}
	void
PutTxLoopFlag(tn_id,flag)	/* Put Loop Flag in text node. */
TN_Id tn_id;			/* Node in which to insert flag. */
boolean flag;			/* Flag status. */

{extern void fatal();		/* Handles fatal errors; in common. */
 extern void fprinst();		/* Prints instructions; in Mach. Dep. */

 if(tn_id->op != LOOP)				/* If not a LOOP node, */
	{fprinst(stderr,-2,tn_id);		/* complain. */
	 fatal("PutTxLoopFlag: above line not a LOOP node.\n");
	}

 tn_id->addrs[3] = (AN_Id) ((flag) ? 1 : 0);

 return;
}


	void
PutTxLoopType(tn_id,looptype)	/* Put Loop Type in text node. */
TN_Id tn_id;			/* Node to use. */
LoopType looptype;		/* Type of LOOP node. */

{extern void fatal();		/* Handles fatal errors; in common. */
 extern void fprinst();		/* Prints an instruction; in Mach. Dep. */

 if(tn_id->op != LOOP)				/* If not a LOOP node, */
	{fprinst(stderr,-2,tn_id);		/* complain. */
	 fatal("PutTxLoopType: above line not a LOOP node.\n");
	}

 tn_id->addrs[0] = (AN_Id) looptype;	/* Put it in the node. */

 return;
}


	void
PutTxOpCodeX(tn_id,op_code)	/* Put operation code index into text node. */
TN_Id tn_id;			/* TN_Id of affected text node. */
unsigned int op_code;		/* Operation code index of affected node. */


{extern void fatal();		/* Handles fatal errors.	*/

 if(op_code > GUPPER)				/* Could this be a valid opcode?*/
	fatal("PutTxOpCodeX: illegal op-code (%u).\n",op_code);
 tn_id->op = (unsigned short)op_code;		/* Insert new op code index. */
 return;
}

#ifndef MACRO
	void
PutTxOperandAd(tn_id,operand,address)	/* Put an address into node. */
TN_Id tn_id;			/* TN_Id of text node. */
unsigned int operand;		/* Index of operand. */
AN_Id address;			/* Operand's address. */


{extern unsigned int Max_Ops;	/* Maximum operands in an instruction.	*/
 extern void fatal();		/* Handles fatal errors; in common.	*/

 if(operand >= Max_Ops)				/* Is operand legal? */
	fatal("PutTxOperandAd: operand illegal (%u).\n",operand);

 tn_id->addrs[operand] = address;		/* Yes: put in address. */

 return;
}
#endif

	void
PutTxOperandFlags(tn_id,operand,flags)	/* Set flags for operand. */
TN_Id tn_id;			/* TN_Id of text node. */
unsigned int operand;		/* Index of operand. */
unsigned flags;			/* Operand's flags. */


{extern unsigned int Max_Ops;	/* Maximum operands in an instruction.	*/
 extern void fatal();		/* Handles fatal errors; in common.	*/

 if(operand >= Max_Ops)				/* Is operand legal? */
	fatal("PutTxOperandFlags: operand illegal (%u).\n",operand);

 tn_id->flags[operand] = (unsigned char)flags;

 return;
}

#ifndef MACRO
	void
PutTxOperandType(tn_id,operand,type)	/* Put an type into node. */
TN_Id tn_id;			/* TN_Id of text node. */
unsigned int operand;		/* Index of operand. */
OperandType type;		/* Operand's type. */


{extern unsigned int Max_Ops;	/* Maximum operands in an instruction.	*/
 extern void fatal();		/* Handles fatal errors; in common.	*/

 if(operand >= Max_Ops)				/* Is operand legal? */
	fatal("PutTxOperandType: operand illegal (%u).\n",operand);

 tn_id->types[operand] = (unsigned char) type;	/* Put in type. */

 return;
}
#endif

#ifndef MACRO
	void
PutTxOperandVol(tn_id,operand,flag)	/* Set/unset volatile bit for operand. */
TN_Id tn_id;			/* TN_Id of text node. */
unsigned int operand;		/* Index of operand. */
boolean flag;		/* Operand's type. */


{extern unsigned int Max_Ops;	/* Maximum operands in an instruction.	*/
 extern void fatal();		/* Handles fatal errors; in common.	*/

 if(operand >= Max_Ops)				/* Is operand legal? */
	fatal("PutTxOperandVol: operand illegal (%u).\n",operand);

 if(flag)
	tn_id->flags[operand] |= VOLATILE;	/* Set volatile bit. */
 else
	tn_id->flags[operand] &= ~VOLATILE;	/* Turn it off. */

 return;
}
#endif
	void
PutTxProtected(tn_id,status)	/* Set Text Node's protected flag. */
TN_Id tn_id;			/* TN_Id of text node. */
boolean status;			/* How it should be set. */

{
 if(status)
	tn_id->protected = 1;
 else
	tn_id->protected = 0;
 return;
}


	void
PutTxRMode(tn_id,mode)		/* Put floating point rounding mode in instr.*/
TN_Id tn_id;			/* TN_Id of text node.	*/
RoundMode mode;			/* Rounding mode for this instruction.	*/

{
 tn_id->RoundMode = (unsigned) mode;		/* Yes: put it in.	*/
 return;
}
	void
PutTxSPI(tnid,flag)		/* Sets Stack Pointer Increment flag.	*/
TN_Id tnid;
boolean flag;

{extern void fatal();		/* Handles fatal errors.	*/

 if(tnid == (TN_Id) NULL)
	fatal("PutTxSPI: null text node identifier.\n");
 if(flag)
	tnid->StackPtrInc = 1;
 else
	tnid->StackPtrInc = 0;
 return;
}


	void
PutTxUniqueId(tn_id,unique)	/* Set Text Node's UniqueId value. */
TN_Id tn_id;			/* TN_Id of text node. */
unsigned long int unique;	/* Unique identifier to assign. */


{
 tn_id->uniqueid = unique;			/* Insert the value. */
 return;
}
	void
WriteTxNode(tn_id,stream)	/* Write text node to a stream.	*/
TN_Id tn_id;			/* TN_Id of node to write.	*/
FILE *stream;			/* Stream pointer of stream on which to write.*/

{
 (void)fwrite(tn_id,sizeof(struct node),1,stream);
 return;
}
	void
TN_INIT()			/* Initialize for text nodes. */

{extern TN_Id FirstTNId;	/* TN_Id of first text node.	*/
 extern TN_Id LastTNId;		/* TN_Id of last text node.	*/
 extern NODE n0;		/* The first text node: not real.	*/
 extern NODE ntail;		/* The last text node: not real.	*/


 FirstTNId = &n0;
 n0.forw = &ntail;				/* Initialize text node list. */
 n0.back = (TN_Id) NULL;
 n0.op = BLOWER;				/* ("UNDEFINED") */
 LastTNId = &ntail;
 ntail.forw = (TN_Id) NULL;
 ntail.back = &n0;
 ntail.op = TAIL;
 return;
}
	void
textaudit(title)		/* Checks text nodes.	*/
char *title;			/* Title for error message.	*/

{extern unsigned int Max_Ops;	/* Maximum number of operands.	*/
#ifdef AUDIT
 boolean failure;		/* TRUE if text audit failure.	*/
 extern void fatal();		/* Handles fatal errors.	*/
 extern void fprinst();		/* Prints a text node.	*/
#if 0
 extern boolean legalgen();	/* TRUE if legal generic supplied.	*/
#endif
 extern NODE n0;		/* Pre first node.	*/
 extern NODE ntail;		/* Post last node.	*/
 boolean ntail_flag;		/* TRUE if ntail seen.	*/
 register unsigned int operand;	/* Operand counter.	*/
 register TN_Id tn_id;		/* Text node id for scanning.	*/

 failure = FALSE;				/* No failures yet.	*/
 ntail_flag = (n0.forw == &ntail) ? TRUE : FALSE;	/* Empty list is OK. */
 for(ALLN(tn_id))				/* Scan all the text nodes. */
	{if(GetTxOpCodeX(tn_id->forw) == TAIL)	/* Got to legal end? */
		ntail_flag = TRUE;
	 if(IsTxAuditerr(tn_id))		/* Have we already complained */
		continue;			/* about this? Once is enough.*/
	 if(tn_id->forw->back != tn_id)		/*Is this node before */
		{(void) fprintf(stdout,
			"textaudit: %s\n\tnode:\t\t",title);
		 fprinst(stdout,-1,tn_id);
		 (void) fprintf(stdout,"\tpoints to:\t");
		 fprinst(stdout,-1,tn_id->forw);
		 (void) fprintf(stdout,"\tbut that points back to:\t");
		 fprinst(stdout,-1,tn_id->forw->back);
		 PutTxAuditerr(tn_id,TRUE);	/* We complained about this. */
		 failure = TRUE;
		} /* END OF if(tn_id->forw->back != tn_id) */

	 if(tn_id->op > GUPPER)			/* Is op-code legal?	*/
		{(void) fprintf(stdout,"textaudit: %s\n\tillegal op-code:\n",
			title);
		 fprinst(stdout,-1,tn_id);
		 PutTxAuditerr(tn_id,TRUE);	/* We complained about this. */
		 failure = TRUE;
		}
#if 0
	 if(IsOpGeneric(tn_id->op))		/* If generic opcode, test */
		{if(!legalgen(tn_id->op,	/* to see if legal. */
				GetTxOperandType(tn_id,0),
				GetAdMode(GetTxOperandAd(tn_id,0)),
				GetTxOperandType(tn_id,1),
				GetAdMode(GetTxOperandAd(tn_id,1)),
				GetTxOperandType(tn_id,2),
				GetAdMode(GetTxOperandAd(tn_id,2)),
				GetTxOperandType(tn_id,3),
				GetAdMode(GetTxOperandAd(tn_id,3))))
			{(void) fprintf(stdout,		/* Not legal if here. */
				"textaudit: %s\n\tillegal node:\t",title);
			 fprinst(stdout,-1,tn_id);
			 PutTxAuditerr(tn_id,TRUE);	/* We complained  */
							/* this already. */
			 failure = TRUE;
			}
		} /* END OF if(IsOpGeneric(tn_id->op)) */
#endif

	 for(operand = 0; operand < Max_Ops; operand++)	/* Check operands. */
		{if((int) tn_id->types[operand] > (int) Tfp)
							/* Types. */
			{(void) fprintf(stdout,
				"textaudit: %s\n\tOperand %u has bad type.\n",
				title,operand);
			 fprinst(stdout,-1,tn_id);
			 PutTxAuditerr(tn_id,TRUE);	/* We complained */
							/* this already. */
			 failure = TRUE;
			}
		 if(GetTxOpCodeX(tn_id) != LOOP
				&& !IsAdValid(tn_id->addrs[operand]))	/* Addresses.	*/
			{(void) fprintf(stdout,
				"textaudit: %s\n\tOp %u has bad addr (%u).\n",
				title,operand,(unsigned)(tn_id->addrs[operand]));
			 fprinst(stdout,-1,tn_id);
			 PutTxAuditerr(tn_id,TRUE);	/* We complained */
							/* about this already.*/
			 failure = TRUE;
			}
		} /* END OF for(operand = 0; operand < Max_Ops; operand++) */
	} /* END OF for(ALLN(tn_id)) */
 if(ntail_flag == FALSE)
	{(void) fprintf(stdout,"textaudit: %s\n\tpremature end of text list.\n",
		title);
	 failure = TRUE;
	}

 if(failure)
	fatal("textaudit: text node trouble.\n");
#endif /*AUDIT*/
 return;
}
