/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)newoptim:common/loop.c	1.7"

#include	<stdio.h>
#include	"defs.h"
#include	"debug.h"
#include	"RegId.h"
#include	"ANodeTypes.h"
#include	"ANodeDefs.h"
#include	"OpTabTypes.h"
#include	"olddefs.h"
#include	"optab.h"
#include	"OperndType.h"
#include	"RoundModes.h"
#include	"LoopTypes.h"
#include	"TNodeTypes.h"
#include	"TNodeDefs.h"
#include	"ALNodeType.h"
#include	"FNodeTypes.h"
#include	"FNodeDefs.h"

static char *LICMM = {"loop invariant code motion"};
static char *Mblicm = {"loop: before loop invariant code motion"};

		/* Private function declarations. */

STATIC TN_Id corrbegin();	/* Finds corresponding loop begin.	*/
STATIC TN_Id corrcond();	/* Finds corresponding loop condition.	*/
STATIC void loopclean();	/* Cleans up initial test if not needed. */
STATIC TN_Id nextend();		/* Finds next loop end.	*/
STATIC unsigned int setdepth();	/* Sets loop depths in LOOP nodes. */


	boolean
loop()				/* Loop Optimizer.	*/
				/* This subroutine picks out the loops to be */
				/* improved and calls the loopopt function to */
				/* do the actual improvements.	*/
				/* It returns if improvements worked	*/
				/* correctly and calls fatal if trouble was */
				/* encountered.	*/

{extern FN_Id FuncId;		/* Id of current function.	*/
 extern unsigned int LICMrem;	/* Counter of number of instructions removed. */
 extern void addrprint();	/* Prints address table.	*/
 TN_Id begin;			/* First item in a loop.	*/
 extern enum CC_Mode ccmode;	/* cc -X? */
 STATIC TN_Id corrbegin();	/* Finds corresponding loop begin.	*/
 TN_Id end;			/* Last item in a loop.	*/
 extern void fatal();		/* Deals with fatal errors.	*/
 extern void fprinst();		/* Prints instruction with a new-line.	*/
 extern void funcprint();	/* Prints a function.	*/
 boolean innerdone;		/* Did we finish with inner loop? */
 STATIC void loopclean();	/* Cleans up initial test if not needed. */
 extern int loopopt();		/* Optimizes a single loop.	*/
 boolean moved;			/* TRUE if instrs got moved out of loops */
 STATIC TN_Id nextend();	/* Finds next loop end.	*/
 extern void prafter();		/* Prints instructions after a change.	*/
 STATIC unsigned int setdepth();	/* Sets loop depths in LOOP nodes. */
 extern TN_Id skipprof();	/* Skips over profiler nodes.	*/
 int status;			/* Return value from some function calls. */

 if(Xskip(XLICM))                               /* Should this optimization */
        return FALSE;                           /* be done? NO. */

 if(DBdebug(3,XLICM))				/* Print address table	*/
	addrprint(Mblicm);			/* if desired.	*/
 if(DBdebug(1,XLICM))				/* Print function	*/
	funcprint(stdout,Mblicm,1);		/* if desired.	*/

 if(setdepth() == 0)				/* Sets LOOP node depths. */
	return FALSE;				/* Quit if no loops.	*/

 /* Function level constraints that must be met before attempting LICM: */
 /* Function must not have calls to setjmp() or ASM's. */
 if(IsFnBlackBox(FuncId))		
	return FALSE;			
 if(ccmode == Transition && IsFnSetjmp(FuncId))		
	return FALSE;			
					/* Start at first non-profiler node. */
 end = skipprof(GetTxNextNode((TN_Id) 0));

						/* DO ONE LOOP AT A TIME. */
 /* Design constraint: attempt LICM only on innermost loop. */
 innerdone = FALSE;
 moved = FALSE;
 while((end = nextend(end)) != NULL)		/* Find (another) loop ending.*/
	{if(GetTxLoopDepth(end) == 1)	/* Outermost loop? */
		innerdone = FALSE;		/* Yes: reset innerdone */
	 if(!(begin = corrbegin(end)))		/* Find corresponding begin? */
		{fprinst(stderr,-2,end);	/* No: show instruction. */
		 fatal("loop: found no corresponding begin for above.\n",0);
		}
	 if(!innerdone)			/* Do serious loop optimiz? */
		{if((status = loopopt(begin,end)) == 0)	/* Yes: do it. */
						/* loopopt() returns -1 if trouble;
					 	 * else returns #instructions moved
						 */
			loopclean(begin);	/* Didn't optimize: clean up.*/
		 else if(status < 0)		/* Trouble?	*/
			{fprinst(stderr,-2,begin);	/* Error.	*/
			 fatal("loop: loopopt failed (%d).\n",status);
			}
		 else
			{moved = TRUE;		/* Note that something got moved */
			 LICMrem += status;	/* Increment number removed.*/
			 if(DBdebug(2,XLICM))	/* Some did move.	*/
				prafter(GetTxPrevNode(begin),
					GetTxNextNode(end),1);
			}
		 innerdone = TRUE;
		}
	 else
		loopclean(begin);		/* No: just clean it up. */
	}

 if(DBdebug(2,XLICM))				/* Print function after	*/
	funcprint(stdout,"loop: after loop invariant code motion",1);

 return(moved);					/* Normal return.	*/
}
	STATIC TN_Id
corrbegin(start)		/* Find the loop begin (LOOP LBEG) node */
				/* before the node pointed to by the argument */
				/* that has the same depth; it corresponds */
				/* to this loop end (LOOP LEND) node */
				/* and return a pointer to it.	*/
				/* If none is found, return the NULL pointer. */
register TN_Id start;		/* Pointer to node where to start looking. */

{register int depth;		/* Depth of end loop.	*/

 depth = GetTxLoopDepth(start);		/* Get loop depth we want to match. */
 while((start = GetTxPrevNode(start)) != NULL)	/* Quit if we get to start of program.*/
	{if((GetTxOpCodeX(start) == LOOP)	/*Is this loop indicator*/
			&& (GetTxLoopType(start) == Begin) /* for beginning */
						/* and depth corresponding? */
			&& (GetTxLoopDepth(start) == depth))
		return(start);		/* Yes: return pointer to caller. */
	}

 return(NULL);				/* No (more) loop beginning found. */
}


	STATIC TN_Id
corrcond(start)			/* Find the loop condition (LOOP LCOND) node */
				/* after the node pointed to by the argument */
				/* that has the same depth; it corresponds */
				/* to this loop condition (LOOP LCOND) node */
				/* and return a pointer to it. If none is */
				/* found, return the NULL pointer.	*/
register TN_Id start;		/* Pointer to node where to start looking. */

{register int depth;		/* Depth of beginning loop.	*/

 depth = GetTxLoopDepth(start);		/* Get loop depth we want to match. */
 while((start = GetTxNextNode(start)) != NULL)	/* Quit if we get to end of program.*/
	{if((GetTxOpCodeX(start) == LOOP)	/* Is this loop indicator */
						/* for condition and	*/
			&& (GetTxLoopType(start) == Condition)	/* depth */
						/* corresponding?	*/
			&& (GetTxLoopDepth(start) == depth))
		return(start);		/* Yes: return pointer to caller. */
	}

 return(NULL);					/* No loop condition found. */
}
	STATIC void
loopclean(start)		/* Remove the loop condition test at the */
				/* head of the loop whose loop beginning */
				/* node (LOOP Begin) is pointed to by the */
				/* argument; replace it with a jump to the */
				/* loop condition test at the end of the */
				/* loop marked by the LOOP Condition node. */
register TN_Id start;		/* Start of loop to clean up.	*/

{STATIC TN_Id corrcond();	/* Finds corresponding loop condition.	*/
 extern void fatal();		/* Handles fatal errors.	*/
 extern void prafter();		/* Prints instructions after a change.	*/
 extern void prbefore();	/* Prints instructions before a change.	*/
 extern void fprinst();		/* Prints instruction with a new-line.	*/
 TN_Id labelp;			/* Pointer to label of loop condition.	*/
 TN_Id next;
 register unsigned int num_rem;	/* Number of pre-loop cond inst removed. */
 extern void PutP();		/* Puts jump target in node.	*/
 int depth = GetTxLoopDepth(start);/* Get loop depth we want to match. */

 labelp = corrcond(start);			/* Get loop label TN_Id. */
 if(!labelp)					/* Did we get it? */
	{fprinst(stderr,-2,start);		/* No: error. */
	 fatal("loopclean: couldn't find condition for above.\n");
	}
 labelp = GetTxNextNode(labelp);	/* The label is the node after LCOND.*/
 if(!IsTxLabel(labelp))			/* Is this a label? */
	return;				/* No label there: cannot cleanup */

 num_rem = 0;

 start = GetTxNextNode(start);
 while(start)					/* Delete pre-loop condition. */
	{next = GetTxNextNode(start);
	 if(!((GetTxOpCodeX(start) == LOOP)	/* End of pre-loop condition? */
			&& (GetTxLoopType(start) == Header)
			&& (GetTxLoopDepth(start) == depth)))
		{if(DBdebug(3,XLICM))
			prbefore(GetTxPrevNode(start),
				GetTxNextNode(start),0,LICMM);
		 DelTxNode(start);		/* No: delete line of code. */
		 if(DBdebug(3,XLICM))
			prafter(GetTxPrevNode(next),next,0);
		 num_rem += 1;			/* Count number deleted. */
		 start = next;			/* Now do the next one.	*/
		 continue;
		}
	 if(num_rem < 1)			/* Did we delete any? */
		return;			/* No, there were none; no jump req'd.*/
	 if(DBdebug(2,XLICM))
		prbefore(GetTxPrevNode(start),
			GetTxNextNode(start),0,LICMM);
						/* Yes: make new jump node. */
	 start = MakeTxNodeBefore(start,IJMP);
	 PutP(start,GetTxOperandAd(labelp,0));	/* Put in jump destination. */
	 if(DBdebug(3,XLICM))
		prafter(GetTxPrevNode(start),
			GetTxNextNode(GetTxNextNode(start)),0);
	 return;				/* Normal return. */
	}

 fprinst(stderr,-2,start);			/* Error if here.	*/
 fatal("loopclean: never found loop header for above.\n");
 /*NOTREACHED*/
}
	STATIC TN_Id
nextend(start)			/* Find next loop ending.	*/
				/* start points to any text node; */
				/* this function returns a pointer to the */
				/* next loop ending (LOOP LEND) node that */
				/* we wish to process.	*/
				/*If none is found, returns the NULL pointer.*/
register TN_Id start;		/* TN_Id of node where to start looking. */

{
 while((start = GetTxNextNode(start)) != NULL)	/* Quit if we get to end of program. */
	{if(GetTxOpCodeX(start) != LOOP)	/* Is this loop indicator?*/
		continue;			/* No: look at next.	*/
	 if(GetTxLoopType(start) == End)	/* Loop end?	*/
		return(start);		/* Yes: return pointer to caller. */
	}

 return(NULL);				/* No (more) loop endings found. */
}


	void
remLOOP()			/* Removes all LOOP nodes.	*/

{register TN_Id ip;		/* Pointer to each node.	*/
 register TN_Id prev_ip;	/* Pointer to previous node.	*/
 extern void prafter();		/* Prints instructions after a change.	*/
 extern void prbefore();	/* Prints instructions before a change.	*/
 extern TN_Id skipprof();	/* Skips over profiler nodes.	*/

 for(ALLNSKIP(ip))				/* Scan all the nodes. */
	{if(GetTxOpCodeX(ip) != LOOP)		/* Is it an LOOP node? */
		continue;			/* No: skip it. */
	 prev_ip = GetTxPrevNode(ip);
	 if(DBdebug(4,XLICM))
		prbefore(GetTxPrevNode(ip),GetTxNextNode(ip),0,LICMM);
	 DelTxNode(ip);				/* Yes: delete it.	*/
	 ip = prev_ip;
	 if(DBdebug(4,XLICM))
		prafter(GetTxPrevNode(ip),GetTxNextNode(ip),0);
	}

 return;
}
	STATIC unsigned int
setdepth()		/* Sets depths of LOOP nodes. */
				/* This function scans the entire function */
				/* and sets the loop nesting level */
				/* appropriate to each LOOP node into */
				/* each LOOP node. The function */
				/* returns number of LOOP nodes encountered. */

{register unsigned int counter;	/* Counts number of LOOPs encountered.	*/
 register unsigned int depth;	/* Loop nesting depth.	*/
 register TN_Id ip;		/* Instruction identifier.	*/
 extern TN_Id skipprof();	/* Skips over profiler nodes.	*/

 counter = 0;					/* No LOOPs yet encountered. */
 depth = 0;					/*Depth 0 is out of any loops.*/

 for(ALLNSKIP(ip))				/* Insert Loop Nesting Levels.*/
	{if(GetTxOpCodeX(ip) != LOOP)		/* Is this a loop marker? */
		continue;			/* No: skip it. */
	 counter += 1;			/* Yes: increment number of LOOPs. */
	 if(GetTxLoopType(ip) == Begin)		/* Is it a begin? If so, */
		depth += 1;			/* increase depth if possible.*/
	 PutTxLoopDepth(ip,depth);		/* Insert loop nesting depth. */
						/* Insert loop serial number */
						/* needed only for debugging. */
	 PutTxLoopSerial(ip,counter);
	 if(GetTxLoopType(ip) == End)		/* Is it an end? */
		depth -= 1;			/* Yes: we are coming out. */
	}
 return(counter);				/* Normal return. */
}
