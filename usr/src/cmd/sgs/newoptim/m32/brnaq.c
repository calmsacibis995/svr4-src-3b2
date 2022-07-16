/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)newoptim:m32/brnaq.c	1.10"


#include	<stdio.h>
#include	<malloc.h>
#include	<string.h>
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

#define	MPA_SOURCE	2	/*Number of source operand for MOVA and PUSHA.*/
#define	DESTINATION	3	/* Number of destination operand.	*/

/* return values for traceback */
#define UNKNOWN		0	/* can't trace back */
#define IMMEDIATE	1	/* traced back to immediate */
#define MPADAPFP	2	/* traced back to mova/pusha of n(%ap/%fp) */

static boolean AnyCalls;	/* there's a subroutine jump or call */
static boolean AnyBadextderefs;	/* there's a "bad" deref to externs */
static boolean AnyBadstkderefs;	/* there's a "bad" deref to stack */
static boolean AnyMPADAP;	/* there's a mova/pusha of n(%ap) */
static boolean AnyMPADFP;	/* there's a mova/pusha of n(%fp) */

	/* Private function declarations. */
STATIC boolean overlaps_others();/* Does address overlap other stack locs? */
STATIC unsigned short traceback();


	void
BRNAQScan(begin,end)
/* Do preliminary scan of basic block delimited by text nodes, begin
 * and end, to flag possible aliasing problems.  This info is used
 * later by IsBRNAQ().  This function should be called before
 * calling IsBRNAQ() to check addresses within this basic block, and
 * it should be called after major transformations to the block.
 * (We may be able to easily generalize BRNAQScan() and IsBRNAQ for
 * use on arbitrary sequences of instructions.)
 */
TN_Id begin;			/* Text node before the block */
TN_Id end;			/* Text node after the block */
{
 extern boolean AnyCalls;
 extern boolean AnyBadextderefs;
 extern boolean AnyBadstkderefs;
 extern boolean AnyMPADAP;
 extern boolean AnyMPADFP;
 extern unsigned int Max_Ops;	/* Maximum operands in an instruction. */
 extern boolean IsDispAPFP();	/* Is address a displacemt of %ap or %fp? */
 STATIC unsigned short traceback();

 TN_Id ip;
 AN_Id an_id;
 unsigned int opcode;
 unsigned int operand;
 extern void prbefore();
 RegId regid;

/* Initialize flags */
 AnyCalls = FALSE;
 AnyBadextderefs = FALSE;
 AnyBadstkderefs = FALSE;
 AnyMPADAP = FALSE;
 AnyMPADFP = FALSE;

 for(ip=GetTxNextNode(begin); ip!=end; ip=GetTxNextNode(ip))
	{
	 opcode = GetTxOpCodeX(ip);

	/* Flag subroutine jumps and calls */
	 if(AnyCalls == FALSE && IsTxAnyCall(ip))
		{
		 AnyCalls = TRUE;
		 if(DBdebug(4,XLICM_1))
			{prbefore(GetTxPrevNode(ip),GetTxNextNode(ip),0,"BRNAQScan");
			 printf("%cAnyCalls = TRUE\n",ComChar);
			}
		}

	/* Flag mova/pusha of n(%fp) and mova/pusha of n(%ap) */
	 if( (opcode == G_MOVA || opcode == G_PUSHA) &&
	    (AnyMPADFP == FALSE || AnyMPADAP == FALSE) ){
		an_id = (GetTxOperandAd(ip,MPA_SOURCE));
		if(IsAdDisp(an_id)){
		    if((regid = GetAdRegA(an_id)) == CFP)
			{
			 AnyMPADFP = TRUE;
			 if(DBdebug(4,XLICM_1))
				{prbefore(GetTxPrevNode(ip),GetTxNextNode(ip),
					0,"BRNAQScan");
				 printf("%cAnyMPADFP = TRUE\n",ComChar);
				}
			}
		    else if (regid == CAP)
			{
			 AnyMPADAP = TRUE;
			 if(DBdebug(4,XLICM_1))
				{prbefore(GetTxPrevNode(ip),GetTxNextNode(ip),
					0,"BRNAQScan");
				 printf("%cAnyMPADAP = TRUE\n",ComChar);
				}
			}
		}
	}
	/* Flag "bad" dereferences to externs and SVs: */
	/* Check any address that is an indirect mode */
	/* (*n(%reg), *e, or n(%rm) ). */
	/* 	If the pointer cannot be traced back to an immediate */
	/* 	or mova/pusha of a displacemt from %ap or %fp, */
	/* 		then we have a possible dereference to externs. */
	/* 	If the pointer cannot be traced back to an immediate, */
	/* 		then we have a possible dereference to stack. */
	if(AnyBadextderefs == TRUE && AnyBadstkderefs == TRUE)
		continue;
	for(ALLOP(ip,operand))
		{
		 an_id = GetTxOperandAd(ip,operand);
		 switch(GetAdMode(an_id))
			{case Disp:
				if(IsDispAPFP(an_id))
					break;
				/*FALLTHRU*/
			 case DispDef:
			 case AbsDef:
				switch(traceback(ip,GetAdUsedId(an_id,0)))
					{case IMMEDIATE:
						break;
					 case MPADAPFP:
						AnyBadstkderefs = TRUE;
						if(DBdebug(4,XLICM_1))
							{prbefore(GetTxPrevNode(ip),
								GetTxNextNode(ip),0,
								"BRNAQScan");
							 printf("%cAnyBadstkderefs = TRUE\n",ComChar);
							}
						break;					
					 case UNKNOWN:
						AnyBadstkderefs = TRUE;
						AnyBadextderefs = TRUE;
						if(DBdebug(4,XLICM_1))
							{prbefore(GetTxPrevNode(ip),
								GetTxNextNode(ip),0,
								"BRNAQScan");
							 printf("%cAnyBadstkderefs = TRUE\n",ComChar);
							 printf("%cAnyBadextderefs = TRUE\n",ComChar);
							}
						break;
					}
			default:	
				break;
			}
		}
	}
}	
	STATIC unsigned short
traceback(ip,apu)		/* Trace an address back to instruction */
				/* that set it. */

/* Scan all instructions i from node before ip backwds
 *	if i is the top of the basic block
 *		return UNKNOWN, since we can't trace any further.
 *	if the dest of i is the address apu (the one we are tracing),
 *		then if i is a move instr and the src of i is immediate,
 *			then return IMMEDIATE
 *		else if i is a move instr and the src of i is NAQ,
 *			then call traceback(i,src of i) to trace back
 *			     src of i, and return that.
 *		else if i is a mova instr and the src of i is a disp of %ap/%fp,
 *			then return MPADAPFP
 *		else
 *			return UNKNOWN, since we don't know what's going on.
 *	if the dest of i is otherwise used by the address we are tracing,
 *		return UNKNOWN, since we don't know what's going on.
 *	if we run out of instructions,
 *		return UNKNOWN, since we can't trace any further.
 */
register TN_Id ip;		/* Node of instruction containing the operand.*/
AN_Id apu;			/* address to trace back */
{
 extern boolean IsDispAPFP();
 register AN_Id dst_id, src_id;		/* src and dest of an instr */
 extern void prbefore();

 if(DBdebug(4,XLICM_1))
	prbefore(GetTxPrevNode(ip),GetTxNextNode(ip),0,"traceback");

 while((ip = GetTxPrevNode(ip)) != NULL)
	{if (IsTxLabel(ip) || IsTxBr(ip))
		return(UNKNOWN);	/* hit top of basic block. */
	 if (apu == (dst_id = GetTxOperandAd(ip,DESTINATION)))
		break;			/* Found where apu is set. */
	 if (IsAdUses(apu,dst_id))
		return(UNKNOWN);	/* Set of something used by apu. */
	}

 if (ip == NULL)
	return(UNKNOWN);	/* Ran out of instructions in function */

 src_id = GetTxOperandAd(ip,MPA_SOURCE);
 if(GetTxOpCodeX(ip) == G_MOV)		/* Is it a G_MOV instruction? */
	{if(IsAdImmediate(src_id))	/* Yes."&n" or "&expr" const? */
		return(IMMEDIATE);	/* Yes: a good destination. */
	 if(IsAdNAQ(src_id))		/* If this is NAQ,see who set.*/
		 return(traceback(ip,src_id));
	}

 if(GetTxOpCodeX(ip) == G_MOVA)		/* Addresses of some are constant.*/
	if(IsDispAPFP(src_id))
		return(MPADAPFP);

 return(UNKNOWN);			/* Afraid to go back except for moves.*/
}
	boolean
IsBRNAQ(ip,operand,begin,end)	/* Is the operand a "block-relative" NAQ? */
/* Checks if address is aliased within the basic block delimited by
 * text nodes, begin and end, based on information gathered by BRNAQScan().
 */
TN_Id ip;			/* Instruction containing operand */
unsigned int operand;		/* Index of operand in instruction */
TN_Id begin, end;		/* Instrs. before and after the block */
{
 STATIC boolean overlaps_others();/* Does address overlap other stack locs? */
 AN_Id an_id = GetTxOperandAd(ip,operand);
 extern unsigned int praddr();
 RegId regid;

 switch(GetAdGnaqType(an_id))
	{
	 case NAQ:	/* a NAQ is not aliased anywhere in the function */
			/* so it is not aliased in the block. */
			return TRUE;
	 case SNAQ:	/* a SNAQ or SENAQ is not aliased if there are */
	 case SENAQ:	/* no subroutine calls or subroutine jumps within */
			/* block. technically, a SNAQ can be aliased only */
			/* if there is a recursive call in the block. */
			/* since we're not tracing call-chains, we assume */
			/* the worst. (this definition of */
			/* aliasing may be LICM-specific). */
			if (AnyCalls)
				{
				 if(DBdebug(4,XLICM_3))
					{(void)praddr(an_id,GetTxOperandType(ip,operand),stdout);
					 printf(":\tIsBRNAQ returns FALSE (SENAQ and calls)\n");
					}
				 return FALSE;
				}
			return TRUE;
	 case ENAQ:	/* Not aliased if there are no subroutine calls or */
			/* subroutine jumps within block (this definition of */
			/* aliasing may be LICM-specific) and there are no */
			/* "bad" dereferences to externs within block. */
			if (AnyCalls || AnyBadextderefs)
				{
				 if(DBdebug(4,XLICM_3))
					{(void)praddr(an_id,GetTxOperandType(ip,operand),stdout);
					 printf(":\tIsBRNAQ returns FALSE (ENAQ and calls or badderefs)\n");
					}
				 return FALSE;
				}
			return TRUE;
	 case SV:	/* Not aliased if all the following are true: */
			/* 1) there are no "bad" dereferences to the */
			/*    stack within block, */
			/* 2) either
			/*	- there is no mova or pusha within block */
			/*	  whose src uses the same reg as used by */
			/*	  this SV, or */
			/*	- there are no subroutine jumps or calls */
			/*	     within block, and */
			/* 3) it has an explicit constant displacement, */
			/* 4) it does not overlap with other stack addresses */
			/*    within block. */

			if (AnyBadstkderefs)
				{if(DBdebug(4,XLICM_3))
					{(void)praddr(an_id,GetTxOperandType(ip,operand),stdout);
				 	 printf(":\tIsBRNAQ returns FALSE (SV and bad derefs)\n");
					}
				 return FALSE;
				}
			regid = GetAdRegA(an_id);
			if (((regid == CFP && AnyMPADFP) ||
			     (regid == CAP && AnyMPADAP)) &&
			    AnyCalls)
				{if(DBdebug(4,XLICM_3))
					{(void)praddr(an_id,GetTxOperandType(ip,operand),stdout);
				 	 printf(":\tIsBRNAQ returns FALSE (SV and mova/pusha's and calls)\n");
					}
				 return FALSE;
				}
			if (!IsAdNumber(an_id))
				{
				 if(DBdebug(4,XLICM_3))
					{(void)praddr(an_id,GetTxOperandType(ip,operand),stdout);
					 printf(":\tIsBRNAQ returns FALSE (SV w/o explicit displacemt)\n");
					}
				 return FALSE;
				}
			if (overlaps_others(ip,operand,begin,end))
				{
				 if(DBdebug(4,XLICM_3))
					{(void)praddr(an_id,GetTxOperandType(ip,operand),stdout);
					 printf(":\tIsBRNAQ returns FALSE (SV overlaps another)\n");
					}
				 return FALSE;
				}
			return TRUE;
	 default:	if(DBdebug(4,XLICM_3))
				{(void)praddr(an_id,GetTxOperandType(ip,operand),stdout);
				 printf(":\tIsBRNAQ returns FALSE (default)\n");
				}
			return FALSE;
	}
}
	STATIC boolean
overlaps_others(inst,opno,begin,end)	/*Check if stack addr overlaps another*/
TN_Id inst;			/* TN_Id of current instruction.	*/
unsigned int opno;		/* Number of current address.	*/
TN_Id begin;			/* TN_Id of instr before block.	*/
TN_Id end;			/* TN_Id of instr after block. */

{long int LSB0;			/* Offset of least significant byte.	*/
 long int LSB1;			/* Offset of least significant byte.	*/
 extern unsigned int Max_Ops;	/* Maximum operands in an instruction.	*/
 long int MSB0;			/* Offset of most significant byte.	*/
 long int MSB1;			/* Offset of most significant byte.	*/
 AN_Id an_id;			/* AN_Id of an operand in the loop.	*/
 register unsigned int operand;	/* Operand  counter.	*/
 extern unsigned int praddr();
 RegId regid0;			/* RegId of incoming address node. */
 TN_Id tn_id;			/* TN_Id of an instruction in the loop.	*/
 OperandType type;		/* Type of an operand in the loop.	*/
 unsigned int opcode;
 extern int TySize();		/* Returns size in bytes of a type.	*/
 AN_Id opad = GetTxOperandAd(inst,opno);/* AN_Id of current address.	*/
 OperandType opty = GetTxOperandType(inst,opno);/* Type of current address.*/

 MSB0 = GetAdNumber(opty,opad);			/* Compute bytes used.	*/
 LSB0 = MSB0 + TySize(opty) - 1;
 regid0 = GetAdRegA(opad);
						/* Scan the loop for overlaps.*/
 for(tn_id = GetTxNextNode(begin); tn_id != end; tn_id = GetTxNextNode(tn_id))
	{
	 if ((opcode = GetTxOpCodeX(tn_id)) == STRCPY || opcode == STREND)
				/* Don't know what addresses get used or set */
		return FALSE;	/* by STRCPY and STREND. */
	 for(ALLOP(tn_id,operand))		/* Scan instruction operands.*/
		{if((tn_id == inst) && (opno == operand)) /*Skip us(identity).*/
			continue;
		 an_id = GetTxOperandAd(tn_id,operand);	/* Get next operand. */
		 if(an_id == opad)		/* Identity is not an alias, */
			continue;		/* so it's OK.	*/
		 if(!IsAdDisp(an_id))		/* If this is not a 	*/
			continue;		/* displacement from	*/
		 if(GetAdRegA(an_id) != regid0)	/* the same register,	*/
			continue;		/* it is OK.	*/
		 type = GetTxOperandType(tn_id,operand);
		 MSB1 = GetAdNumber(type,an_id);	/* Compute bytes used.*/
		 LSB1 = MSB1 + TySize(type) - 1;
		 if(LSB0 < MSB1)
			continue;
		 if(LSB1 < MSB0)
			continue;
		 if(DBdebug(4,XLICM_3))
			{(void)printf("%coverlaps_others:\t",ComChar);
			 (void)praddr(opad,opty,stdout);
			 (void)printf(" overlaps ");
			 (void)praddr(an_id,type,stdout);
			 (void)putchar(NEWLINE);
			}
		 return(TRUE);			/* Overlap: fear aliasing. */
		} /* END OF for(ALLOP(tn_id,operand)) */
	} /* END OF for(tn_id = header; tn_id != end; tn_id = GetTxNext... */
 return(FALSE);					/* No overlap detected.	*/
}
	boolean
mem_const(ip,operand)	/* Determines if some label is a constant in memory. */
/* An address node is a memory constant if the following are true:
 * 1) it is used in the absolute mode in the instruction,
 * 2) it is of the form ".XXX",
 * 3) a) it is in readonly memory,
 *	OR
 *    b.1) its address and the addresses of other nodes of the form
 *       ".XXX+E" have not been taken, as evidenced by both 
 *	 i) none of these being used in the immediate mode, and
 *	 ii) none of these being used as the source of mova or pusha
 *	AND
 *    b.2) it and other nodes of the form ".XXX+E" are not used as the
 *       destination of an instruction. 
 *
 * (We may catch more than memory constants here, but that's okay.)
 */
TN_Id ip;
unsigned int operand;

{
 AN_Id an_id = GetTxOperandAd(ip,operand);
 OperandType type = GetTxOperandType(ip,operand);
 register AN_Id Ean_id;		/* Extra AN_Id of an address node. */
 extern unsigned int Max_Ops;	/* Maximum operands in an instruction.	*/
 char *expression;		/* Place for an expression.	*/
 char *expression2;		/* Place for another expression. */
 extern void fatal();
 unsigned int opnd;
 extern TN_Id skipprof();	/* Skips the label, save, and profiling code. */
 register TN_Id tn_id;		/* Extra TN_Id of an instruction node. */
 unsigned int lbl_len;
 static char *Alpha_Num_Dot =
	{".LI0123456789ABCDEFGHJKMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"};

 expression = GetAdExpression(type,an_id);
 if(!(IsAdAbsolute(an_id) && (*expression == '.')))
	return(FALSE);		/* Not absolute or of form ".XXX" */
 if(IsAdRO(an_id))
	 return(TRUE);		/* A read-only label */
 if(IsAdProbe(an_id,Immediate))
	 return(FALSE);		/* Address has been taken */

 lbl_len = strspn(expression,Alpha_Num_Dot);
				/* Make a duplicate of the */
				/* expression since the next call */
				/* to GetAdExpression() will */
				/* overwrite it. */
 expression = strdup(expression);
 if(expression == NULL)
	fatal("mem_const: out of space\n");

 for(ALLNSKIP(tn_id))				/* No-one may take address.*/
	{if(IsTxAux(tn_id))			/* None in aux nodes. */
		continue;
	 for(ALLOP(tn_id,opnd))
						/* Want address node. */
		{Ean_id = GetTxOperandAd(tn_id,opnd);
						/* If someone takes its */
						/* address,it cannot be a NAQ.*/
						/* We worry about initialized */
						/* arrays. Any displacement */
						/* from a label must be */
						/* suspect. */
		 if(!(IsAdImmediate(Ean_id) ||	/* Labels whose address is */
				IsAdAbsolute(Ean_id)))	/* taken are */
			continue;		/* immediate: label. */
		 expression2 = GetAdExpression(GetTxOperandType(tn_id,opnd),Ean_id);
		 if(strspn(expression2,Alpha_Num_Dot) != lbl_len)
			continue;		/* Label different enough. */
		 if(strncmp(expression,expression2,(int) lbl_len) == 0)
			{if(IsAdImmediate(Ean_id))	/* &Label? */
				{Free(expression);
				 return(FALSE);	/* Fear address taken.*/
				}
			 if(opnd == MPA_SOURCE)	/* Special for mova */
				{if((GetTxOpCodeX(tn_id) == G_MOVA)
						|| (GetTxOpCodeX(tn_id) ==
							G_PUSHA))
					{Free(expression);
					 return(FALSE);	/* Fear address taken.*/
					}
				}
			 if(opnd == DESTINATION)	/* Is it destination? */
				{Free(expression);
				 return(FALSE);	/* Fear address taken.*/
				}
			}
		}
	}
 Free(expression);
 return(TRUE);

}
