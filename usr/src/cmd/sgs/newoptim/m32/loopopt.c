/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)newoptim:m32/loopopt.c	1.10"

#include	<stdio.h>
#include	"defs.h"
#include	"debug.h"
#include	"optab.h"
#include	"OpTabTypes.h"
#include	"OpTabDefs.h"
#include	"RegId.h"
#include	"ANodeTypes.h"
#include	"ANodeDefs.h"
#include	"olddefs.h"
#include	"LoopTypes.h"
#include	"OperndType.h"
#include	"RoundModes.h"
#include	"TNodeTypes.h"
#include	"TNodeDefs.h"

#define	MAXLOOP		263	/* Max # instrs inside body of a loop*/
#define	MPA_SOURCE	2	/*Number of source operand for MOVA and PUSHA.*/
#define	DESTINATION	3	/* Number of destination operand.	*/
#define NVectors	((LICM_SIZE + WDSIZE - 1) / WDSIZE)
#define WITHINTRACKINGRANGE(a)	(IsAdAddrIndex(a)&&(GetAdAddrIndex(a)<nbits))
		/* tests if address a is one of the bits being tracked by */
		/* the following arrays. */

static char *reason = {"unknown"};	/* Reason for changing instr to CGI. */

static enum inst_group {
	IGI,		/* Invariant Group Instruction */
	CGI		/* Complement Group Instruction */
} mark[MAXLOOP];	/* Mark each instruction as IGI or CGI */

/* The following bit vectors are computed by loopopt() and used by
 * failsIGIconstraints(). 
 */
static unsigned long SetS[MAXLOOP][NVectors];
			/* addresses set by an instruction in loop */
static unsigned long UseS[MAXLOOP][NVectors];
			/* addresses used by an instruction in loop */
static unsigned long OSetS[NVectors];
			/* all the addresses set in loop */
static unsigned long OUseS[NVectors];
			/* all the addresses used in loop */
static unsigned long U_B_Set[NVectors];
			/* addresses used before being set in loop */
static unsigned long OCGI_Set[NVectors];
			/* addresses set by CGI's in loop */
static unsigned long Later_Set[NVectors];
			/* addresses set after a given instruction in loop */
static unsigned int nvectors;
			/* number of bit vectors in use */
static unsigned int nbits;
			/* number of bits in use */

	/* Private function declarations. */
STATIC boolean badbranch();	/* TRUE if branch other that at loop end. */
STATIC boolean badlabel();	/* TRUE if label in middle of "block".	*/
STATIC boolean notloop();	/* TRUE if really not a loop.	*/
STATIC boolean case2();		/* Case 2 of CGI constraint.	*/
STATIC boolean case3();		/* Case 3 of CGI constraint.	*/
STATIC boolean case4();		/* Case 4 of CGI constraint.	*/
STATIC boolean failsIGIconstraints();
				/* Checks if instr. fails constraints to */
				/* preserve reaching defs. */
STATIC TN_Id findheader();	/* Finds next header node.	*/

	int
loopopt(begin,end)		/* OPTIMIZE A LOOP.	*/
				/* Returns number of lines moved from loop; */
				/* this value may be 0.	*/
				/* Returns -1 if terrible trouble.	*/
TN_Id begin;			/* TN_Id of loop begin mark node.	*/
TN_Id end;			/* TN_Id of loop end mark node.	*/

{
 STATIC TN_Id findheader();	/* Finds next header node.	*/
 STATIC boolean badbranch();	/* TRUE if branch other that at loop end. */
 STATIC boolean badlabel();	/* TRUE if label in middle of "block".	*/
 STATIC boolean notloop();	/* TRUE if really not loop. */
 extern void prbefore();	/* Prints nodes before change.	*/
 extern void ldbits();		/* Prints out addrs from a bit-vector */
 extern void sets();		/* Gets all sets information for a node. */
 extern void uses();		/* Gets all uses information for a node. */
 STATIC boolean failsIGIconstraints();
				/* Checks if instr. fails constraints to */
				/* preserve reaching defs. */
 extern boolean mem_const();	/* Checks if operand is a memory constant */
 extern void BRNAQScan();
 extern boolean IsBRNAQ();
 extern unsigned int Max_Ops;	/* Maximum operands in an instruction. */
 extern char *reason;		/* Reason for changing instr to CGI. */
 extern unsigned int GnaqListSize;    /* Current size of GNAQ list.   */
 TN_Id header;			/* TN_Id of loop header. */
 TN_Id afterheader;		/* TN_Id of first instr. in loop body. */
 TN_Id beforeend;		/* TN_Id of last instr. in loop body. */
 register unsigned int ins_no;	/* Loop instruction number.	*/
 unsigned int endndx;		/* index of #LOOP END instruction.*/
 register TN_Id ip;		/* TN_Id of instruction being examined. */
 AN_Id an_id;			/* Address node being examined. */
 unsigned int level;		/* Level of loop we are optimizing */
 unsigned int op_code;		/* Operation code index.	*/
 unsigned int operand;		/* Operand counter for instructions.	*/
 register unsigned int vector;	/* Vector number for sets-uses info.	*/
 unsigned int num_moved;	/* Number of instructions moved. */
 RegId regid;			/* Register identifier */
 boolean newCGI = FALSE;	/* Flag if this iteration produced new CGI. */

 if((header = findheader(begin)) == (TN_Id) NULL)/* Find the loop header. */
	return(-1);			/* Couldn't find it: trouble. */
 afterheader = GetTxNextNode(header);	/* First instr in loop body. */
 beforeend = GetTxPrevNode(end);	/* Last instr in loop body. */
 level = GetTxLoopDepth(begin);		/* Level of loop we are optimizing */

/* FIRST PASS. Is loop too hard?
 * Design constraints that must be met to bound the problem:
 * 1) loop body has less than or equal to MAXLOOP instructions, and
 * 2) body of the loop is a single basic block.
 */
 for(ip=afterheader, ins_no=0; ip!=end; ip=GetTxNextNode(ip), ins_no++)
	{
	 if(ins_no >= MAXLOOP)		/*Is loop too big to improve? */
		{if(DBdebug(3,XLICM_1))
			printf("%cloopopt: loop too big.\n",ComChar);
		 return(0);			/* Yes: quit: this is OK. */
		}
	 if(badbranch(ip,level))	/* Tolerate branch for loop */
		{if(DBdebug(3,XLICM_1))
			printf("%cloopopt: badbranch:quitting.\n",ComChar);
		 return(0);	/* only. Quit: loop too complex: OK. */
		}
	 if(badlabel(ip,level))		/*Quit if bad label .*/
		{if(DBdebug(3,XLICM_1))
			printf("%cloopopt: badlabel: quitting.\n",ComChar);
		 return(0);
		}
	 if(notloop(ip,level))		/*Quit if not loop .*/
		{if(DBdebug(3,XLICM_1))
			printf("%cloopopt: notloop: quitting.\n",ComChar);
		 return(0);
		}
	}
 endndx = ins_no;		/* index of #LOOP END instruction */
 if(DBdebug(3,XLICM_1))
	{printf("%cloopopt: pass 1: loop contains %d instructions and,\n",
		ComChar,endndx+1);
	 printf("%cno bad branches or labels.\n",ComChar);
	}
/* INITIALIZE.	*/
						/* Compute number of vectors */
						/* to use.	*/
 nbits = GnaqListSize;
 if(nbits > LICM_SIZE)
	nbits = LICM_SIZE;
 nvectors = (nbits + WDSIZE - 1) / WDSIZE;

 for(vector = 0; vector < nvectors; vector++)
	{OSetS[vector] = 0;			/* Addresses set in loop. */
	 OUseS[vector] = 0;			/* Addresses used in loop. */
	 U_B_Set[vector] = 0;			/* addresses used before set. */
	 OCGI_Set[vector] = 0;			/* addresses set by CGIs. */
	 Later_Set[vector] = 0;			/* addresses set after a given instr*/
	}
 for(ins_no = 0; ins_no < endndx; ins_no++)
	mark[ins_no] = IGI;	/* Assume all instrs. are invariant first. */

 BRNAQScan(header,end);

/* SECOND PASS. Compute the necessary preliminary info.
 * SetS[i] - addresses set by instr. i.
 * UseS[i] - addresses used by instr. i.
 * OSetS - addresses set in the loop.
 * OUseS - addresses used in the loop.
 * U_B_Set - addresses used in loop before being set in loop.
 */
 for(ip=afterheader, ins_no=0; ip!=end; ip=GetTxNextNode(ip), ins_no++)
	{
	 sets(ip,SetS[ins_no],nvectors);	/* Get sets information. */
	 uses(ip,UseS[ins_no],nvectors);	/* Get uses information. */
	 for(vector = 0; vector < nvectors; vector++)
		{OUseS[vector] |= UseS[ins_no][vector];
		 U_B_Set[vector] |= SetS[ins_no][vector] &
				    (OUseS[vector] & (~OSetS[vector]));
		 OSetS[vector] |= SetS[ins_no][vector];
		}
	 if(DBdebug(4,XLICM_2))
		{prbefore(GetTxPrevNode(ip),GetTxNextNode(ip),0,"loopopt pass 2");
		 printf("%c\tUseS[%u]:",ComChar,ins_no);
		 ldbits(stdout,UseS[ins_no],nbits);
		 printf("\n%c\tSetS[%u]:",ComChar,ins_no);
		 ldbits(stdout,SetS[ins_no],nbits);
		 putchar(NEWLINE);
		}
	}
 if(DBdebug(4,XLICM_2))
	{printf("%cList of OUseS (used in loop) after pass 2.\n",ComChar);
	 ldbits(stdout,OUseS,nbits);
	 printf("\n%cList of OSetS (set in loop) after pass 2.\n",ComChar);
	 ldbits(stdout,OSetS,nbits);
	 printf("\n%cList of U_B_Set (used before set) after pass 2.\n",ComChar);
	 ldbits(stdout,U_B_Set,nbits);
	 putchar(NEWLINE);
	 putchar(NEWLINE);
	}
/* THIRD PASS. Identify all CGI's based on single instr. constraints:
 * Scan all instructions i from top of loop body to bottom
 * 	If either
 * 	- i is related to flow of control, or
 * 	- i is protected, or
 *	- i contains volatile references, or
 *	- i uses or sets an address that is not constant or BRNAQ,
 * 	Then mark i CGI,
 *	     set newCGI to TRUE, and
 *	     add what i sets to OCGI_Set.
 */
 for(ip=afterheader, ins_no=0; ip!=end; ip=GetTxNextNode(ip), ins_no++)
	{
	 if(DBdebug(4,XLICM_3))
		{prbefore(GetTxPrevNode(ip),GetTxNextNode(ip),0,"loopopt: pass 3");
			 printf("%cbeing considered.\n",ComChar);
		}

	/* Instructions related to flow of control: */
	 if((IsTxBr(ip)) ||
	    (IsTxLabel(ip)) ||
	    ((op_code = GetTxOpCodeX(ip)) == LOOP) ||
	    (op_code == G_PUSH) ||
	    (op_code == G_PUSHA) ||
	    (IsTxAnyCall(ip)))
		{if(DBdebug(4,XLICM_3))
			reason = "related to flow of control";
		 goto IsCGI;
		}
	/* Protected instructions */
	 if(IsTxProtected(ip))
		{if(DBdebug(4,XLICM_3))
			reason = "protected";
		 goto IsCGI;
		}

	/* Instructions that contain volatile operands, and 
	 * use or set addresses that are not constant or BRNAQ.
	 */
	 for(ALLOP(ip,operand))
		{
		 if(IsTxOperandVol(ip, operand)){
			if(DBdebug(4,XLICM_3))
				reason = "volatile operand";
			goto IsCGI;
		 }
		 an_id = GetTxOperandAd(ip,operand);
		 if(IsAdImmediate(an_id))
			continue;	/* a constant */
		 if((GetTxOpCodeX(ip) == G_MOVA) &&
			  (operand == MPA_SOURCE) &&
			  (IsAdDisp(an_id)) &&
			  ((regid = GetAdRegA(an_id)) == CFP || regid == CAP))
			continue;	/* a closet constant */
			
		 if(WITHINTRACKINGRANGE(an_id)
		    && IsBRNAQ(ip,operand,header,end))
			/* We check to see if we track it in UseS */
			/* and SetS. */
			continue;	/* a BRNAQ */

		 if(mem_const(ip,operand))
			continue;	/* constant in memory */

		/* None of the above, so we consider it CGI. */
		if(DBdebug(4,XLICM_3))
			reason = "address out of tracking range, not BRNAQ, or not constant";
		goto IsCGI;
		}

	/* Not a CGI */
	if(DBdebug(4,XLICM_3))
		printf("%cInstruction stays IGI\n\n",ComChar);
	 continue;

IsCGI:	/* Mark it CGI, set newCGI, and add what instr set to OCGI_Set. */
	 mark[ins_no] = CGI;
	 newCGI = TRUE;
	 for(vector = 0; vector < nvectors; vector++)
		OCGI_Set[vector] |= SetS[ins_no][vector];
	 if(DBdebug(4,XLICM_3))
		{printf("%cInstruction changed to CGI (%s).\n\n",ComChar,reason);
		 printf("\n%cList of OCGI_Set (set by CGIs).\n",ComChar);
		 ldbits(stdout,OCGI_Set,nbits);
		 putchar(NEWLINE);
		}
	}

/* FOURTH PASS: use constraints for preserving reaching definitions
 * to find rest of CGI's:
 * while (newCGI)
 *	set newCGI to FALSE
 *	set Later_Set to 0
 *	Scan all instructions i from bottom of loop body to top
 *		if i is currently IGI but fails IGI constraints
 *			then mark i CGI,
 *			     set newCGI to TRUE, and
 *			     add what i set to OCGI_Set
 *		add what i set to Later_Set
 */
 while (newCGI)
	{
	 newCGI = FALSE;
	 for(vector = 0; vector < nvectors; vector++)
		Later_Set[vector] = 0;
	 for(ip=beforeend, ins_no=endndx-1; ip!=header; ip=GetTxPrevNode(ip), ins_no--)
		{if(mark[ins_no] == IGI && failsIGIconstraints(ins_no,endndx))
			{mark[ins_no] = CGI;
			 newCGI = TRUE;
			 for(vector = 0; vector < nvectors; vector++)
				OCGI_Set[vector] |= SetS[ins_no][vector];
			 if(DBdebug(4,XLICM_4))
				{prbefore(GetTxPrevNode(ip),GetTxNextNode(ip),
					0,"loopopt: pass 4");
				 printf("%cInstruction changed to CGI (%s).\n\n",
					ComChar,reason);
				 printf("\n%cList of OCGI_Set (set by CGIs).\n",ComChar);
				 ldbits(stdout,OCGI_Set,nbits);
				 putchar(NEWLINE);
				}
			}
		 for(vector = 0; vector < nvectors; vector++)
			Later_Set[vector] |= SetS[ins_no][vector];
		}
	}		 

/* FIFTH PASS: move IGI instructions before the loop header in
 * the same relative order. */
 num_moved = 0;
 for(ip=afterheader, ins_no=0; ip!=end; ins_no++)
	{if(mark[ins_no] == IGI)		/* Will this be moved? */
		{TN_Id curr_ip = ip;
		 if(DBdebug(3,XLICM_5))
			{prbefore(GetTxPrevNode(ip),GetTxNextNode(ip),0,
				"loopopt: pass 5");
			 printf("%cwill move.\n\n",ComChar);
			}
		 ip = GetTxNextNode(ip);
		 PutTxUniqueId(curr_ip,IDVAL);	/* Delete line number.*/
		 (void) MoveTxNodeBefore(curr_ip,header);	/*Yes:move it.*/
		 num_moved += 1;			/* Increment number moved. */
		}
	 else
		{if(DBdebug(3,XLICM_5))
			{prbefore(GetTxPrevNode(ip),GetTxNextNode(ip),0,
				"loopopt: pass 5");
			 printf("%cwon't move.\n\n",ComChar);
			}
		 ip = GetTxNextNode(ip);
		}
	}
 if(DBdebug(1,XLICM_5) && (num_moved != 0))
    prbefore(GetTxPrevNode(begin),GetTxNextNode(end),0,"loopopt");

 return(num_moved);			/* return # instructions moved */
}
	STATIC TN_Id
findheader(start)		/* Finds the next node that is a */
				/* for-loop header; returns NULL */
				/* pointer if it cannot find it. */

register TN_Id start;			/* Where to start looking. */

{
 while(GetTxNextNode(start))			/* Find the loop header. */
	{if((GetTxOpCodeX(start) != LOOP)	/* Find LOOP node with */
			|| (GetTxLoopType(start) != Header))	/* Header type. */
		{start = GetTxNextNode(start);	/* This isn't it. */
		 continue;			/* Try the next. */
		}
	 else
		return(start);			/* Found loop heaader. */
	}
 return(NULL);					/* Couldn't find it: trouble. */
}
	STATIC boolean
badbranch(ip,level)		/* TRUE for any branch other than the last */
				/* one in the loop condition area. */
				/* FALSE for non-branches and the branch */
				/* just before the loop end. */
register TN_Id ip;		/* TN_Id of an instruction. Assumes that */
				/* instr. is betw #LOOP HDR and #LOOP END. */
unsigned int level;		/* Level we are working at. */

{unsigned int depth;		/* Depth of current item. */

 if(IsTxAux(ip))				/* If this is an aux node, */
	return(FALSE);				/* destination irrelevant. */
 if(!IsTxBr(ip))				/* If this is not a jump, OK. */
	return(FALSE);				/* Not a jump. */
 ip = GetTxNextNode(ip);			/* Pointer to next node. */
 if(GetTxOpCodeX(ip) != LOOP)		/* A jump: is next one the loop end? */
	return(TRUE);				/* No: this is a bad branch. */
 depth = GetTxLoopDepth(ip);			/* Get depth of next node. */
 if((GetTxLoopType(ip) == End)
		&& (depth == level))
	return(FALSE);				/* Yes: this is a good branch.*/
 return(TRUE);					/* No: this is a bad branch.*/
}

	STATIC boolean
badlabel(ip,level)		/* Returns TRUE if node is a label in the */
				/* "middle" of our loop. */
register TN_Id ip;		/* Pointer to node to be tested. */
unsigned int level;		/* Loop depth we are working on. */

{unsigned int depth;			/* Depth of LOOP node. */
 LoopType looptype;			/* Type of LOOP node.	*/

 if(!IsTxLabel(ip))				/* Is this a label at all? */
	return(FALSE);				/* No: no more testing needed.*/
 ip = GetTxPrevNode(ip);			/* Look at previous node. */
 if(GetTxOpCodeX(ip) != LOOP)		/* Only loop indicating nodes allowed.*/
	return(TRUE);			/* This isn't one of them: bad label. */
 depth = GetTxLoopDepth(ip);			/* Get the depth of this item.*/
 looptype = GetTxLoopType(ip);
 if((looptype == Header)			/* Of these, only loop-header */
		&& (depth == level))
	return(FALSE);				/* is good; */
 if((looptype == Condition)		/* and loop-condition is also good. */
		&& (depth == level))
	return(FALSE);
 if((looptype == End)				/* Belongs to inner loop? */
		&& (depth > level))
	return(FALSE);				/* Then it's OK. */
 return(TRUE);				/* Anything else is bad. */
}
	STATIC boolean
notloop(ip, level)		/* Returns TRUE if a nonloop.		*/
				/* We assume that if there's no code	*/
				/* between a #LOOP COND and a #LOOP END,*/
				/* then this is either a NULL loop, e.g.*/
				/* while(0){...}, or nothing interesting*/
				/* is going on.				*/
register TN_Id ip;		/* Pointer to node to be tested. */
unsigned int level;		/* Loop depth we are working on. */

{
 if(GetTxOpCodeX(ip) != LOOP)		/* Is this a #LOOP? */
	return(FALSE);			/* No: it's o.k. */
 if(GetTxLoopDepth(ip) != level)	/* are we at the same level ? */
	return(FALSE);			/* No, it's o.k. */
 if(GetTxLoopType(ip) != Condition)	/* Is this a #LOOP COND? */
	return(FALSE);			/* No, it's o.k. */
 ip = GetTxNextNode(ip);		/* Look at next node. */
 if(GetTxOpCodeX(ip) != LOOP)		/* Is the next node a #LOOP? */
	return(FALSE);			/* No, it's o.k. */
 if(GetTxLoopType(ip) == End)		/* Is the next node a #LOOP END? */
	return(TRUE);			/* YES, bad loop! */
 return(FALSE);				/* Anything else is o.k. */
}
	STATIC boolean
case2(i)	/* Does an instruction i use an address that */
			/* is last set by a CGI? */
/* Compute LUseS: addresses used by i and set by a CGI.
 * Scan backwds for all instructions u from i to top of loop body
 *	if u is CGI and u sets an address from LUseS
 *		then return TRUE
 *	if all the instrs that set the addresses from LUseS are found,
 *		then return FALSE
 * Return FALSE (since no CGI's set any addresses from LUseS)
 */
unsigned int i;				/* index of instruction in loop */
{
 extern enum inst_group mark[MAXLOOP];
 extern unsigned long SetS[MAXLOOP][NVectors];
 extern unsigned long UseS[MAXLOOP][NVectors];
 extern unsigned long OCGI_Set[NVectors];
 unsigned long int LUseS[NVectors];
 register unsigned int ins_no, vector;
 unsigned long anymore = 0;	/* Is LUseS a vector of 0's? */

/* Compute LSetS: addresses used by i and set by a CGI */
 for(vector = 0; vector < nvectors; vector++)
	anymore |=
		(LUseS[vector] = UseS[i][vector] & OCGI_Set[vector]);

 if(anymore == 0)
	return FALSE;	/* No addresses used by i and set by a CGI */

 if (i == 0)
	return FALSE;	/* Top of loop body - not previously set in loop */

 ins_no=i-1;
 do
	{
	 if(mark[ins_no] == CGI)
		for(vector = 0; vector < nvectors; vector++)
			if (SetS[ins_no][vector] & LUseS[vector])
				/* found CGI that sets something from LUseS*/
				return TRUE;
	 anymore = 0;
	 for(vector = 0; vector < nvectors; vector++)
		/* Remove addresses that are set from LUseS */
		(anymore |= (LUseS[vector] &= ~SetS[ins_no][vector]));
	 if (anymore == 0)
		return FALSE;	/* found all instrs that set what's in LUseS*/
	}
 while (ins_no-- != 0);

 return FALSE;		/* didn't find any CGI's that set what's in LUseS*/
}

	STATIC boolean
case3(i,endndx)	/* Does an instruction i set an address that */
			/* is also set by a CGI and that reaches */
			/* a use after i that is CGI or is outside the loop? */
/* Compute LSetS: addresses set by i and by a CGI.
 * Scan all instructions u from i+1 down to bottom of loop
 *	if u is CGI and u uses an address from LSetS
 *		then return TRUE
 *	if all addresses from LSetS are dead after u
 *		then return FALSE (since we found all uses reached by i)
 * Return TRUE (since i reaches uses outside the loop)
 */
unsigned int i;				/* index of instruction in loop */
unsigned int endndx;			/* index of #LOOP END instr. */
{
 extern enum inst_group mark[MAXLOOP];
 extern unsigned long SetS[MAXLOOP][NVectors];
 extern unsigned long UseS[MAXLOOP][NVectors];
 extern unsigned long OCGI_Set[NVectors];
 unsigned long int LSetS[NVectors];
 register unsigned int ins_no, vector;
 unsigned long anymore = 0;	/* Is LSetS a vector of 0's? */

/* Compute LSetS: addresses set by i and by a CGI */
 for(vector = 0; vector < nvectors; vector++)
	anymore |= (LSetS[vector] = 
			SetS[i][vector] & OCGI_Set[vector]);

 if(anymore == 0)
	return FALSE;	/* No addresses set by i and by a CGI */

 for(ins_no=i+1; ins_no<endndx; ins_no++)
	{
	 if(mark[ins_no] == CGI)
		for(vector = 0; vector < nvectors; vector++)
			if (UseS[ins_no][vector] & LSetS[vector])
				/* found CGI that uses something from LSetS*/
				return TRUE;
	 anymore = 0;
	 for(vector = 0; vector < nvectors; vector++)
		/* Remove addresses that are set from LSetS */
		(anymore |= (LSetS[vector] &= ~SetS[ins_no][vector]));
	 if (anymore == 0)
		return FALSE;	/* found all sets of what is in LSetS. */
	}

 return TRUE;	/* Some addresses from LSetS are still live at end of loop. */
}

	STATIC boolean
case4(i,endndx)	/* Does an instruction i set an address that */
			/* is later set again and that reaches */
			/* a use after i that is CGI? */
/* Compute LSetS: addresses set by i and later set again.
 * Scan all instructions u from i+1 down to bottom of loop
 *	if u is CGI and u uses an address from LSetS
 *		then return TRUE
 *	if all addresses from LSetS are dead after u
 *		then return FALSE (since we found all uses reached by i)
 */
unsigned int i;				/* index of instruction in loop */
unsigned int endndx;			/* index of #LOOP END instr. */
{
 extern enum inst_group mark[MAXLOOP];
 extern unsigned long SetS[MAXLOOP][NVectors];
 extern unsigned long UseS[MAXLOOP][NVectors];
 extern unsigned long Later_Set[NVectors];
 extern void fatal();
 unsigned long int LSetS[NVectors];
 register unsigned int ins_no, vector;
 unsigned long anymore = 0;	/* Is LSetS a vector of 0's? */

/* Compute LSetS: addresses set by i and later set again */
 for(vector = 0; vector < nvectors; vector++)
	anymore |= (LSetS[vector] = 
			SetS[i][vector] & Later_Set[vector]);
					
 if(anymore == 0)
	return FALSE;	/* No addresses set by i and later set again */

 for(ins_no=i+1; ins_no<endndx; ins_no++)
	{
	 if(mark[ins_no] == CGI)
		for(vector = 0; vector < nvectors; vector++)
			if (UseS[ins_no][vector] & LSetS[vector])
				/* found CGI that uses something from LSetS*/
				return TRUE;
	 anymore = 0;
	 for(vector = 0; vector < nvectors; vector++)
		/* Remove addresses that are set from LSetS */
		(anymore |= (LSetS[vector] &= ~SetS[ins_no][vector]));
	 if (anymore == 0)
		return FALSE;	/* found all sets of what is in LSetS */
	}

/* We should never reach here. */
 fatal("case4 (LICM): logic error\n");
 return FALSE;			/* to shut LINT up */
}

	STATIC boolean
failsIGIconstraints(index,endndx)/* Checks if instr. fails constraints to */
					/* preserve reaching defs. */
/* 1) If an address used or set by the instr. has been used before set in loop, 
 *	then return TRUE
 * 2) If an address used by the instr. is last set by a CGI,
 *	then return TRUE
 * 3) If an address set by the instr. both:
 *	i) is also set by a preceding CGI (we need only check if it's set
 *	   by any CGI) and
 *	ii) reaches a use which is outside the loop or is CGI,
 *	then return TRUE
 * 4) If an address set by the instr. both:
 *	i) reaches a use which is CGI and
 *	i) is later set again,
 *	then return TRUE
 * Else return FALSE
 */
unsigned int index;		/* index of instruction */
unsigned int endndx;		/* index of #LOOP END instr. */
{
 STATIC boolean case2();
 STATIC boolean case3();
 STATIC boolean case4();
 extern unsigned long SetS[MAXLOOP][NVectors];
 extern unsigned long UseS[MAXLOOP][NVectors];
 extern unsigned long U_B_Set[NVectors];
 extern char *reason;		/* Reason for changing instr to CGI. */
 register unsigned int vector;

 for(vector = 0; vector < nvectors; vector++)
	if(U_B_Set[vector] & (UseS[index][vector] | SetS[index][vector]))
		{reason = "used before set";
		 return TRUE;				/* case 1 */
		}
 if(case2(index))
	{reason = "last set by CGI";
	 return TRUE;					/* case 2 */
	}
 if(case3(index,endndx))
	{reason = "set by preceding CGI and reaches use outside loop or by CGI";
	 return TRUE;					/* case 3 */
	}
 if(case4(index,endndx))
	{reason = "reaches use by CGI and reset later";
	 return TRUE;					/* case 4 */
	}
 return FALSE;
}
