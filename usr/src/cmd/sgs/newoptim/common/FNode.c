/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)newoptim:common/FNode.c	1.8"

#include	<stdio.h>
#include	<malloc.h>
#include	<string.h>
#include	"defs.h"
#include	"ANodeTypes.h"
#include	"OpTabTypes.h"
#include	"OpTabDefs.h"
#include	"olddefs.h"
#include	"LoopTypes.h"
#include	"OperndType.h"
#include	"optab.h"
#include	"RegId.h"
#include	"RoundModes.h"
#include	"ANodeDefs.h"
#include	"ALNodeType.h"
#include	"ALNodeDefs.h"
#include	"TNodeTypes.h"
#include	"TNodeDefs.h"
#include	"FNodeTypes.h"

static FN_Id FirstFNId = {NULL};	/* FN_Id of first function node. */
static FN_Id LastFNId = {NULL};		/* FN_Id of last function node. */

	/* private functions */
STATIC int FN_Compar();			/* Comparison function for function nodes. */


	void
DoFnAlias(fn_id)		/* Make all aliased REGALs nonNAQs. */
FN_Id fn_id;
{
 register AN_Id an_id;

 DoAlias(fn_id->abegin);

 if(fn_id->PAlias == 0)		/* Has the param stack being aliased? */
	return;			/* No, then all's o.k. */

				/* Make all params nonNAQs. */
 for(an_id = GetAdNextNode((AN_Id)NULL); an_id != NULL; an_id = GetAdNextNode(an_id))
	if(IsAdNAQ(an_id) && IsAdArg(an_id))
		PutAdGnaqType(an_id,SV);
}
	STATIC int
FN_Compar(arg1,arg2)		/* Compares function nodes for sort.	*/
FN_Id *arg1;
FN_Id *arg2;

{
 return((*arg1)->instructions - (*arg2)->instructions);
}
	unsigned long int
GetFnCalls(fn_id)		/* Get number of 1st level call instructions.*/
FN_Id fn_id;			/* FN_Id of function.	*/
{
 return(fn_id->calls);
}

	unsigned long int
GetFnExpansionLimit(fn_id)	/* Get limit on 1st level call expansions */
FN_Id fn_id;			/* FN_Id of function.	*/
{
 return(fn_id->expansion_limit);
}

	unsigned long int
GetFnExpansions(fn_id)		/* Get actual call expansions at all levels */
FN_Id fn_id;			/* FN_Id of function. */
{
 return(fn_id->expansions);
}


	TN_Id
GetFnFirst(fn_id)		/* Get starting TN_Id of function.	*/
FN_Id fn_id;			/* FN_Id of function.	*/
{extern void fatal();		/* Handles fatal errors.	*/

 if(fn_id->start != (TN_Id) NULL)
	return(fn_id->start);			/* Return start if it exists. */
 fatal("StartTxFunction: no first text-node for function: %s.\n",
	GetAdExpression(Tbyte,fn_id->name));
 /*NOTREACHED*/
}
	FN_Id
GetFnId(name)			/* Get FN_Id of function with a name.	*/
AN_Id name;			/* Name of the function.	*/
				/* We return its function-identifier.	*/
{extern FN_Id FirstFNId;	/* FN_Id of first function node.	*/
 extern FN_Id LastFNId;		/* FN_Id of last function node.	*/
 extern int errno;		/* UNIX error number.	*/
 extern void fatal();		/* Handles fatal errors.	*/
 register FN_Id fn_id;
 register FN_Id newone;		/* Header of newly named function.	*/

 for(fn_id = FirstFNId; fn_id; fn_id = fn_id->next)	/* Find the function. */
	if(fn_id->name == name)			/* Is this it?	*/
		return(fn_id);			/* Yes: return fn_id.	*/
 newone = (FN_Id) Malloc(sizeof(struct Function)); /*Get space for function header.*/
 if(newone == NULL)				/* Did we get it?	*/
	fatal("NameTxFunction: Malloc failed (%d) for '%s`.\n",
		errno,GetAdExpression(Tbyte,name));
 newone->name = name;				/* Put in the name.	*/
 newone->next = (FN_Id) NULL;			/* Put us at end of list. */
 newone->prev = LastFNId;
 newone->begin = (AN_Id) NULL;			/* Init Private Node Ids.*/
 newone->start = (TN_Id) NULL;			/*Initialize text identifiers.*/
 newone->finish = (TN_Id) NULL;			/*Initialize text identifiers.*/
 newone->abegin = (ALN_Id) NULL;		/* Init alias list.	*/
 newone->calls = 0;				/* Initialize counters.	*/
 newone->expansion_limit = 0;			/* Initialize counters. */
 newone->expansions = 0;			/* Initialize counters. */
 newone->instructions = 0;			/* Initialize counters.	*/
 newone->auditerr = 0;				/* No error reported.	*/
 newone->blackbox = 0;				/* No black box in func .*/
 newone->defined = 0;				/* Not defined yet.	*/
 newone->library = 0;				/* Not library yet.	*/
 newone->nodblflg = 0;				/* No  NODBL flag seen yet. */
 newone->candidate = 0;				/* Not expansion candidate. */
 newone->LocalSizeFlag = 0;		/* Local Stack Size not present. */
 newone->NumReg = 0;			/* Initialize counter. */
 newone->RVRegs = MXRETREG;	/* Default number of returned registers.*/
 newone->cand_loc_sz = 0;			/* Initialize counter. */
 newone->setjmp = 0;				/* No setjmp found. 	*/
 newone->MISconvert = 0;			/* No #TYPE yet.	*/
 newone->PAlias = 0;				/* Param stack not aliased. */
 if(!FirstFNId)				/* Start linked list if it is empty. */
	FirstFNId = newone;
 else
	LastFNId->next = newone;		/* append to end of list. */
 LastFNId = newone;
 return(newone);
}
	void
GetFnLocNm(fn_id,dest,size)	/* Get Local Stack name.	*/
FN_Id fn_id;			/* Function whose stack name is wanted.	*/
char *dest;			/* Where to put name.	*/
unsigned int size;		/* Name size limit.	*/

{extern void fatal();		/* Handles fatal errors.	*/
 /*extern int strlen();		** Returns string length; in C(3) library. */
 /*extern char *strncpy();	** Counted string copy; in C(3) library. */

 if(fn_id->LocalName[0] == EOS)			/* Is name present?	*/
	fatal("GetFnLocNm: name not present.\n");	/* No: fatal.	*/
 if(strlen(fn_id->LocalName) >= size)		/* Will it fit?	*/
	fatal("GetFnLocNm: name too long.\n");
 (void) strncpy(dest,fn_id->LocalName,MaxLocalName);
 return;
}


	unsigned long int
GetFnLocSz(fn_id)		/* Get Local Stack Size in bytes.	*/
FN_Id fn_id;			/* Function whose size is wanted.	*/

{extern void fatal();		/* Handles fatal errors.	*/

 if(fn_id->LocalSizeFlag == 0)			/* Is size present?	*/
	fatal("GetFnLocSz: size not present.\n");	/* No: fatal.	*/
 return(fn_id->LocalSize);
}

	unsigned long int
GetFnCandLocSz(fn_id)		/* Get Candidate Local Size in bytes.	*/
FN_Id fn_id;			/* Function whose size is wanted.	*/

{
 return(fn_id->cand_loc_sz);
}


	AN_Id
GetFnName(fn_id)		/* Get AN_Id of name of function.	*/
FN_Id fn_id;			/* FN_Id of function whose name is wanted. */

{
 return(fn_id->name);				/* AN_Id of name.	*/
}


	unsigned int
GetFnNumReg(fn_id)		/* Get number of registers used by function. */
FN_Id fn_id;

{
 return(fn_id->NumReg);
}


	unsigned int
GetFnRVRegs(fn_id)	/* Get number of registers returned by function.*/
FN_Id fn_id;

{
 return(fn_id->RVRegs);
}


	void
GetFnPrivate(fn_id)		/* Restore private nodes (inverse of	*/
FN_Id fn_id;			/* PutFnPrivate).	*/

{
 RestoreAdPrivate(fn_id->begin);
 return;
}


	FN_Id
GetFnIdProbe(name)		/* Get FN_Id of function with a name.	*/
AN_Id name;			/* Name of the function.	*/
				/* We return its function-identifier.	*/
{extern FN_Id FirstFNId;	/* FN_Id of first function node.	*/
 register FN_Id fn_id;

 for(fn_id = FirstFNId; fn_id; fn_id = fn_id->next)	/* Find the function. */
	if(fn_id->name == name)			/* Is this it?	*/
		return(fn_id);			/* Yes: return fn_id.	*/
 return((FN_Id) NULL);
}
	unsigned long int
GetFnInstructions(fn_id)	/* Get number of instructions in function. */
FN_Id fn_id;			/* FN_Id of function.	*/

{
 return(fn_id->instructions);
}


	TN_Id
GetFnLast(fn_id)		/* Get TN_Id of last text node of function. */
FN_Id fn_id;			/* FN_Id of function.	*/

{extern void fatal();		/* Handles fatal errors.	*/

 if(fn_id->finish != (TN_Id) NULL)		/* Is it defined?	*/
	return(fn_id->finish);			/* Yes: return finish.	*/
 fatal("GetFnLast: no last text-node for function: %s.\n",
	GetAdExpression(Tbyte,fn_id->name));
 /*NOTREACHED*/
}


	FN_Id
GetFnNextNode(fn_id)		/* Get next function node in list.	*/
FN_Id fn_id;

{extern FN_Id FirstFNId;	/* FN_Id of first function node.	*/

 if(fn_id == (FN_Id) NULL)			/* If null,	*/
	return(FirstFNId);			/* give them the first.	*/
 return(fn_id->next);				/* Otherwise, the next.	*/
}


	FN_Id
GetFnPrevNode(fn_id)		/* Get previous function node in list.	*/
FN_Id fn_id;

{extern FN_Id LastFNId;		/* FN_Id of last function node.	*/

 if(fn_id == (FN_Id) NULL)			/* If null,	*/
	return(LastFNId);			/* give them the last.	*/
 return(fn_id->prev);				/* Otherwise, the previous. */
}

	AN_Id
GetFnNextPrivateAd(fn_id,an_id)	/* Get next in list of private address nodes */
FN_Id fn_id;
AN_Id an_id;

{
 if(an_id == (AN_Id) NULL)
	return(fn_id->begin);
 return(an_id->privt);
}

#ifndef MACRO
	boolean
IsFnAlias(fn_id,an_id)
FN_Id fn_id;
AN_Id an_id;
{
 return( IsAliased(fn_id->abegin,an_id) );
}
#endif

	boolean
IsFnAuditerr(fn_id)		/* Is Func Node's Auditerr flag set? */
FN_Id fn_id;			/* Func node to test.	*/

{
 if(fn_id->auditerr == 1)
	return(TRUE);
 else
	return(FALSE);
}

	boolean
IsFnBlackBox(fn_id)		/* Is Func Node's BlackBox flag set? */
FN_Id fn_id;			/* Func node to test.	*/

{
 if(fn_id->blackbox == 1)
	return(TRUE);
 else
	return(FALSE);
}

	boolean
IsFnCandidate(fn_id)		/* TRUE iff fn_id is a candidate for inline.*/
FN_Id fn_id;

{
 if(fn_id->candidate == 1)
	return(TRUE);
 return(FALSE);
}


	boolean
IsFnDefined(fn_id)		/* TRUE iff fn_id is a defined function.*/
FN_Id fn_id;

{
 if(fn_id->defined == 1)
	return(TRUE);
 return(FALSE);
}

#ifndef MACRO
	boolean
IsFnLibrary(fn_id)		/* TRUE iff fn_id is a library function.*/
FN_Id fn_id;

{
 if(fn_id->library == 1)
	return(TRUE);
 return(FALSE);
}
#endif

#ifndef MACRO
	boolean
IsFnMISconvert(fn_id)		/* TRUE iff fn_id contains a #TYPE.*/
FN_Id fn_id;

{
 if(fn_id->MISconvert == 1)
	return(TRUE);
 return(FALSE);
}
#endif

#ifndef MACRO
	boolean
IsFnPAlias(fn_id)		/* TRUE iff function's param stack's been aliased.*/
FN_Id fn_id;
{
 if(fn_id->PAlias == 1)
	return TRUE;
 return FALSE;
}
#endif

	boolean
IsFnNODBL(fn_id)		/* Is Func Node's NODBL flag set? */
FN_Id fn_id;			/* Func node to test.	*/

{
 if(fn_id->nodblflg == 1)
	return(TRUE);
 else
	return(FALSE);
}

	boolean
IsFnSetjmp(fn_id)		/* TRUE iff fn_id calls setjmp. */
FN_Id fn_id;

{
 if(fn_id->setjmp == 1)
	return(TRUE);
 return(FALSE);
}

#ifndef MACRO
	void
PutFnAlias(fn_id,an_id)		/* insert an_id into fn_id's alias list */
FN_Id fn_id;
AN_Id an_id;
{
 STATIC void PutFnPAlias();

 PutAlias(&fn_id->abegin,an_id);
}
#endif
	void
PutFnAuditerr(fn_id,status)	/* Set Function Node's Auditerr flag. */
FN_Id fn_id;			/* TN_Id of function node. */
boolean status;			/* How it should be set. */

{
 if(status)
	fn_id->auditerr = 1;
 else
	fn_id->auditerr = 0;
 return;
}

	void
PutFnBlackBox(fn_id,status)	/* Set Function Node's BlackBox flag. */
FN_Id fn_id;			/* TN_Id of function node. */
boolean status;			/* How it should be set. */

{
 if(status)
	fn_id->blackbox = 1;
 else
	fn_id->blackbox = 0;
 return;
}

	void
PutFnCalls(fn_id,number)	/* Put number of 1st level calls */
				/* in function node. */
FN_Id fn_id;			/* FN_Id of function.	*/
unsigned long int number;	/* Number of calls.	*/

{
 fn_id->calls = number; 	/* Put it in.	*/
 return;
}

	void
PutFnExpansionLimit(fn_id,number) /* Put limit on 1st level calls */
				/* in function node. */
FN_Id fn_id;			/* FN_Id of function.	*/
unsigned long int number;	/* Number of calls.	*/

{
 fn_id->expansion_limit = number; /* Put it in.	*/
 return;
}

	void
PutFnExpansions(fn_id,number)	/* Put actual number of calls to all levels */
				/* in function node. */
FN_Id fn_id;			/* FN_Id of function.	*/
unsigned long int number;	/* Number of calls.	*/

{
 fn_id->expansions = number;	/* Put it in.	*/
 return;
}


	void
PutFnCandidate(fn_id,flag)	/* Put candidate bit in function node.	*/
FN_Id fn_id;			/* FN_Id of affected node.	*/
boolean flag;			/* Status of affected node.	*/

{
 if(flag)
	fn_id->candidate = 1;
 else
	fn_id->candidate = 0;
 return;
}


	void
PutFnDefined(fn_id,flag)	/* Put defined bit in function node.	*/
FN_Id fn_id;			/* FN_Id of affected node.	*/
boolean flag;			/* Status of affected node.	*/

{
 if(flag)
	fn_id->defined = 1;
 else
	fn_id->defined = 0;
 return;
}


	void
PutFnFunction(fn_id)		/* Put text nodes into function node.	*/
FN_Id fn_id;			/* FN_Id of function.	*/

{extern void TN_INIT();		/* Initialize unnamed function list.	*/

 fn_id->start = GetTxNextNode((TN_Id) NULL);	/* Put function into node. */
 fn_id->finish = GetTxPrevNode((TN_Id) NULL);

 TN_INIT();					/* Re-initialize unnamed list.*/

 return;
}


	void
PutFnInstructions(fn_id,number)	/*Put number of instructions in function node.*/
FN_Id fn_id;			/* FN_Id of function.	*/
unsigned long int number;	/* Number of instructions.	*/

{
 fn_id->instructions = number;			/* Put it in.	*/
 return;
}


	void
PutFnLibrary(fn_id,flag)	/* Put library bit in function node.	*/
FN_Id fn_id;			/* FN_Id of affected node.	*/
boolean flag;			/* Status of affected node.	*/

{
 if(flag)
	fn_id->library = 1;
 else
	fn_id->library = 0;
 return;
}
	void
PutFnLocNm(fn_id,name)		/* Insert Local Stack Name in function.	*/
FN_Id fn_id;			/* Function id.	*/
char *name;			/* Name of local stack variable.	*/

{extern void fatal();		/* Handles fatal errors.	*/
 /*extern char *strncpy();	** Counted string copy; in C(3) library. */
 /*extern int strlen();		** Measures string length; in C(3) library. */

 if((int)strlen(name) >= MaxLocalName)
	fatal("PutFnLocNm: string length too long.\n");
 (void) strncpy(fn_id->LocalName,name,MaxLocalName);
 return;
}


	void
PutFnLocSz(fn_id,size)		/* Insert Local Stack Size in function.	*/
FN_Id fn_id;			/* Function id.	*/
unsigned long int size;		/* Local stack size of function.	*/

{
 fn_id->LocalSize = size;			/* Insert size.	*/
 fn_id->LocalSizeFlag = 1;			/* Mark size present.	*/
 return;
}

#ifndef MACRO
	void
PutFnMISconvert(fn_id,status)	/* Set Function Node's #TYPE flag. */
FN_Id fn_id;			/* TN_Id of function node. */
boolean status;			/* How it should be set. */

{
 if(status)
	fn_id->MISconvert = 1;
 else
	fn_id->MISconvert = 0;
 return;
}
#endif

	void
PutFnNODBL(fn_id,status)	/* Set Function Node's NODBL flag. */
FN_Id fn_id;			/* TN_Id of function node. */
boolean status;			/* How it should be set. */

{
 if(status)
	fn_id->nodblflg = 1;
 else
	fn_id->nodblflg = 0;
 return;
}

	void
PutFnCandLocSz(fn_id,size)	/* Insert Candidate Local Size in funct. */
FN_Id fn_id;			/* Function id.	*/
unsigned long int size;		/* Local stack size of function.	*/

{
 fn_id->cand_loc_sz = size;			/* Insert size.	*/
 return;
}


	void
PutFnNumReg(fn_id,number)	/* Put number of registers used in function. */
FN_Id fn_id;
unsigned int number;

{
 fn_id->NumReg = number;
 return;
}
	void
PutFnRVRegs(fn_id,number) /* Put number of registers returned by function.*/
FN_Id fn_id;
unsigned int number;

{
 fn_id->RVRegs = number;
 return;
}

#ifndef MACRO
	void
PutFnPAlias(fn_id,status)
FN_Id fn_id;
boolean status;
{
	if(status)
		fn_id->PAlias = 1;
	else
		fn_id->PAlias = 0;
}
#endif
	void
PutFnPrivate(fn_id)		/* Put private address nodes in function node */
FN_Id fn_id;			/* and hide them.	*/

{
 fn_id->begin = HideAdPrivateNodes();
 return;
}


	void
PutFnSetjmp(fn_id,flag)		/* Put setjmp bit in function node.	*/
FN_Id fn_id;			/* FN_Id of affected node.	*/
boolean flag;			/* Status of affected node.	*/

{
 if(flag)
	fn_id->setjmp = 1;
 else
	fn_id->setjmp = 0;
 return;
}
	void
DelFnNode(fn_id)		/* Delete a function.	*/
FN_Id fn_id;			/* FN_Id of function to delete.	*/

{extern FN_Id FirstFNId;	/* Id of first function node.	*/
 extern FN_Id LastFNId;		/* Id of last function node.	*/
 extern void fatal();		/* Handles fatal errors.	*/
 FN_Id next;
 FN_Id prev;

 if(fn_id->start)				/* If text nodes, error	*/
	 fatal("DelFnNode: func node has text nodes '%s`.\n",
		GetAdExpression(Tbyte,fn_id->name));
 if(fn_id->begin)				/* If private address */
 						/* nodes, error	*/
	 fatal("DelFnNode: func node has private address nodes '%s`.\n",
		GetAdExpression(Tbyte,fn_id->name));
 prev = fn_id->prev;				/* Remove this function node. */
 next = fn_id->next;
 Free((char *) fn_id);				/* Release its space.	*/
 if(prev)					/* Fix up  linked list.	*/
	prev->next = next;
 else
	FirstFNId = next;
 if(next)
	next->prev = prev;
 else
	LastFNId = prev;

 return;
}
	void
SortFnNodes()			/* Sort the function nodes.	*/

{extern FN_Id FirstFNId;	/* Identifier of first function node.	*/
 extern FN_Id LastFNId;		/* Identifier of last function node.	*/
 STATIC int FN_Compar();	/* Comparison function for function nodes. */
 extern int errno;		/* UNIX error number.	*/
 extern void fatal();		/* Handles fatal errors.	*/
 register FN_Id index;
 register FN_Id *limit;
 unsigned int maxfuncnodes;	/* Function node counter.	*/
 extern void qsort();		/* Sort function; in C(3) library.	*/
 FN_Id *sort_table;		/* Table of function nodes to sort.	*/
 register FN_Id *sti;		/* Sort table index.	*/

 maxfuncnodes = 0;				/* Count function nodes. */
 for(index = FirstFNId; index != (FN_Id) NULL; index = GetFnNextNode(index))
	maxfuncnodes += 1;

						/* Make space for sort table. */
 if(maxfuncnodes == 0)				/* If no function nodes, */
	return;					/* don't bother.	*/

 sort_table = (FN_Id *) Malloc(maxfuncnodes * sizeof(FN_Id));
 if(sort_table == NULL)
	fatal("SortFnNodes: Malloc failed (%d).\n",errno);

 sti = sort_table;
 for(index = FirstFNId; index != (FN_Id) NULL; index = GetFnNextNode(index))
	*sti++ = index;				/* Put FN_Id's in table. */

						/* Sort them.	*/
 qsort((char *) sort_table,maxfuncnodes,sizeof(FN_Id),FN_Compar);

						/* Remake linked list.	*/
 limit = sti - 1 ;
 for(sti = sort_table; sti != (limit + 1); sti++)
	{if(sti != limit)
		(*(sti + 1))->prev = *sti;
	 else
		LastFNId = *sti;
	 if(sti != sort_table)
		(*(sti - 1))->next = *sti;
	 else
		FirstFNId = *sti;
	}
 (*sort_table)->prev = (FN_Id) NULL;
 (*limit)->next = (FN_Id) NULL;

 return;
}

#ifndef MACRO
	void
UndoFnAlias(fn_id)
FN_Id fn_id;
{
	UndoAlias(fn_id->abegin);
	FreeAlias(&fn_id->abegin);
}
#endif
	void
funcaudit(title)		/* Checks function nodes.	*/
char *title;			/* Title for error message.	*/

{
#ifdef MACRO
 extern TN_Id FirstTNId;
 extern NODE ntail;
#endif /*MACRO*/
#ifdef AUDIT
 extern AN_Id GetFnName();	/* Get name of function. */
 extern FN_Id GetFnNextNode();	/* Get next node in function list. */
 extern AN_Id GetFnNextPrivateAd();/* Get next hidden private address node */
 extern boolean IsAdAbsolute();	/* TRUE if address node is Absolute. */
 extern boolean IsFnAuditerr();	/* TRUE if error been reported on func. */
 extern unsigned int Max_Ops;	/* Maximum number of operands.	*/
 AN_Id an_id;			/* Address node scanning variable. */
 register FN_Id fn_id;		/* Function node id for scanning.	*/
 extern void fprinst();		/* Prints a text node.	*/
 extern boolean legalgen();	/* TRUE if legal generic supplied.	*/
 boolean nlast_flag;		/* TRUE if last node seen.	*/
 register unsigned int operand;	/* Operand counter.	*/
 AN_Id opnd;			/* Operand address node. */
 extern unsigned int praddr();	/* Print address node. */
 boolean private_flag;		/* TRUE is address is in private list. */
 register TN_Id tn_id;		/* Text node id for scanning.	*/

 nlast_flag = FALSE;				/* Prepare for nlast test. */
 for(fn_id = GetFnNextNode((FN_Id) NULL); 
		fn_id != (FN_Id) NULL;
		fn_id = GetFnNextNode(fn_id))
						/* Scan all the func nodes. */
	{if(fn_id == LastFNId) 			/* Got to legal end? */
		nlast_flag = TRUE;
	 if(IsFnAuditerr(fn_id))		/* Have we already complained */
		continue;			/* about this? Once is enough.*/
	 if(!IsAdValid(fn_id->name)
			|| !IsAdAbsolute(fn_id->name))	/* Function name. */
		{(void) fprintf(stderr,
			"funcaudit: %s\n\tName has bad addr (%u).\n",
			title,(unsigned)(fn_id->name));
		 PutFnAuditerr(fn_id,TRUE);	/* We complained */
						/* about this already.*/
		 continue;			/* can't print diagnostics */
		}
	 if(fn_id->next != (FN_Id) NULL 
			&& fn_id->next->prev != fn_id)	/*Is this node before */
		{(void) fprintf(stderr,
			"funcaudit: %s\n\fnode:\t\t",title);
		 (void) praddr(GetFnName(fn_id),Tbyte,stderr);
		 (void) fprintf(stderr,"\n");
		 (void) fprintf(stderr,"\tpoints to:\t");
		 (void) praddr(GetFnName(fn_id->next),Tbyte,stderr);
		 (void) fprintf(stderr,"\n");
		 (void) fprintf(stderr,"\tbut that points back to:\t");
		 (void) praddr(GetFnName(fn_id->next->prev),Tbyte,stderr);
		 (void) fprintf(stderr,"\n");
		 PutFnAuditerr(fn_id,TRUE);	/* We complained about this. */
		} /* END OF if(fn_id->next->prev != fn_id) */

	 if(fn_id->defined == 0)
		if(fn_id->begin != NULL
				|| fn_id->start != NULL
				|| fn_id->finish != NULL
				|| fn_id->expansion_limit != 0
				|| fn_id->expansions != 0
				|| fn_id->instructions != 0
				|| fn_id->LocalSizeFlag != 0
				|| fn_id->candidate != 0
				|| fn_id->blackbox != 0
				|| fn_id->MISconvert != 0
				|| fn_id->PAlias != 0
				|| fn_id->setjmp != 0)
			{(void) fprintf(stderr,
				"funcaudit: %s\n\tUndefined func has garbage.\n",					title);
			 PutFnAuditerr(fn_id,TRUE);
			 continue;
			}
		
	 if(fn_id->candidate == 0)		/* rest only for candidates */
		continue;

	 for(tn_id = fn_id->start;
			tn_id != fn_id->finish; 
			tn_id = GetTxNextNode(tn_id))
		{if(tn_id->forw->back != tn_id)		/*Is this node before */
			{(void) fprintf(stderr,
				"funcaudit: %s\n\tnode:\t\t",title);
			 fprinst(stderr,-1,tn_id);
			 (void) fprintf(stderr,"\tpoints to:\t");
			 fprinst(stderr,-1,tn_id->forw);
			 (void) fprintf(stderr,"\tbut that points back to:\t");
			 fprinst(stderr,-1,tn_id->forw->back);
			 PutFnAuditerr(fn_id,TRUE);	/* We complained */
							/* about this. */
			} /* END OF if(tn_id->forw->back != tn_id) */
	
		 if(tn_id->op > GUPPER)			/* Is op-code legal? */
			{(void) fprintf(stderr,
				"funcaudit: %s\n\tillegal op-code:\n",
				title);
			 fprinst(stderr,-1,tn_id);
			 PutFnAuditerr(fn_id,TRUE);	/* We complained */
							/* about this. */
			}
		 if(IsOpGeneric(tn_id->op))		/* If gen opcode,test*/
			{if(!legalgen(tn_id->op,	/* to see if legal. */
					GetTxOperandType(tn_id,0),
					GetAdMode(GetTxOperandAd(tn_id,0)),
					GetTxOperandType(tn_id,1),
					GetAdMode(GetTxOperandAd(tn_id,1)),
					GetTxOperandType(tn_id,2),
					GetAdMode(GetTxOperandAd(tn_id,2)),
					GetTxOperandType(tn_id,3),
					GetAdMode(GetTxOperandAd(tn_id,3))))
				{(void) fprintf(stderr,	/* Not legal if here. */
					"funcaudit: %s\n\tillegal node:\t",
					title);
				 fprinst(stderr,-1,tn_id);
				 PutFnAuditerr(fn_id,TRUE); /* We complained  */
						/* about this already. */
				}
			} /* END OF if(IsOpGeneric(tn_id->op)) */
	
		 if(IsOpAux(tn_id->op))			/* skip these for now */
			continue;
							/* Check operands. */
		 for(operand = 0; operand < Max_Ops; operand++)	
			{if((int) tn_id->types[operand] > (int) Tfp) /*Types.*/
				{(void) fprintf(stderr,
					"funcaudit: %s\n\tOp %u has bad type\n",
					title,operand);
				 fprinst(stderr,-1,tn_id);
				 PutFnAuditerr(fn_id,TRUE); /* We complained */
						/* about this already. */
				}
			 opnd = GetTxOperandAd(tn_id,operand);
							/* Look for address */
							/* in private list. */
			 private_flag = FALSE;		/* Start search. */
			 for(an_id = GetFnNextPrivateAd(fn_id,(AN_Id) NULL);
					an_id != (AN_Id) NULL;
					an_id = GetFnNextPrivateAd(fn_id,an_id))
				if(an_id == opnd)
					private_flag = TRUE;	/* Found it. */
			 if(private_flag == FALSE 
					&&!IsAdValid(opnd))
				{(void) fprintf(stderr,
				   "funcaudit: %s\n\tOp %u has bad addr (%u)\n",
					title,operand,(unsigned)opnd);
				 fprinst(stderr,-1,tn_id);
				 PutFnAuditerr(fn_id,TRUE);  /* We complained */
							/* about this already.*/
				}
			} /* END OF for(operand = 0; operand 
					< Max_Ops; operand++) */
		} /* END OF for(ALLN(tn_id)) */
	} /* END OF for over fn_id */
 if(FirstFNId != (FN_Id) NULL && nlast_flag == FALSE)
	{(void) fprintf(stderr,"funcaudit: %s\n\tpremature end of func list.\n",
		title);
	}
#endif /*AUDIT*/
 return;
}
