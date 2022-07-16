/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)newoptim:m32/w1opt.c	1.10"

/* w1opt.c
**
**	32100 optimizer:  for one-instruction window
**
**
*/

/****************************************************************/
/* Note: The use of goto's in this file instead of long strings */
/* if "if else" clauses is done to increase portability.	*/
/****************************************************************/

#include	<stdio.h>
#include	"defs.h"
#include	"debug.h"
#include	"OpTabTypes.h"
#include	"OpTabDefs.h"
#include	"RegId.h"
#include	"RegIdDefs.h"
#include	"ANodeTypes.h"
#include	"ANodeDefs.h"
#include	"LoopTypes.h"
#include	"OperndType.h"
#include	"olddefs.h"
#include	"RoundModes.h"
#include	"TNodeTypes.h"
#include	"TNodeDefs.h"
#include	"optab.h"
#include	"optim.h"
#include	"optutil.h"

#define	FIRST_SRC	0
#define	SECOND_SRC	1
#define	THIRD_SRC	2
#define	DESTINATION	3

#define LIMIT109	3
/*
** This routine handles the single-instruction optimization window.
** See individual comments below about what's going on.
** In some cases (which are noted), the optimizations are ordered.
*/

	boolean				/* TRUE if we make any changes */
w1opt(pf,peeppass)
register TN_Id pf;			/* pointer to first instruction in
					** window (and last).	*/
unsigned int peeppass;			/* Peephole pass number.	*/

{
 extern AN_Id C;			/* AN_Id of C Condition Code. */
 extern unsigned int CountOneBits();	/* Counts 1-bits in a byte.	*/
 extern long int ExtendItem();		/* Makes item fit specified data size.	*/
 extern AN_Id i0;			/* Immediate 0 address id */
 extern AN_Id i1;			/* Immediate 1 address id */
 extern void ldelin2();
 extern unsigned int ndisc;		/* Number of instructions discarded.	*/
 extern struct opent optab[];		/* opcode table */
 extern int optmode;		/* Mode of optimization: speed, size, etc. */
 extern void peepchange();

 register int opf = GetTxOpCodeX(pf);	/* single instruction's op code # */

 OperandType tyf1 = GetTxOperandType(pf,1); 
 OperandType tyf2 = GetTxOperandType(pf,2); 
 OperandType tyf3 = GetTxOperandType(pf,3); 

 AN_Id adf0 = GetTxOperandAd(pf,0);
 AN_Id adf1 = GetTxOperandAd(pf,1);
 AN_Id adf2 = GetTxOperandAd(pf,2);
 AN_Id adf3 = GetTxOperandAd(pf,3);

 TN_Id LastNode;			/* Last node so far considered.	*/
 long int ShiftAmount;			/* Amount of shift required; e.g., 109.	*/
 AN_Id an_id;				/* Place to keep an address node id: 108.*/
 int chgflg;				/* flag to note change to be made */
 long int b;
 int index;				/* Loop index.	*/
 unsigned short int new_op;		/* Operation code index for new opcode.	*/
 long int result;			/* equivalant constant (107)	*/
 union
	{unsigned int UI[1];
	 unsigned char UC[4];
	} value;	
 long int value1;			/* Value of adf1 sometimes.	*/
 long int value2;			/* Value of adf2 sometimes.	*/

#ifdef W32200
 extern boolean IsAutoAddress();	/* TRUE if auto-address in window. */
 extern boolean IsAutoDependency();	/* TRUE if autodependency.	*/
 boolean AutoAddress;			/* TRUE if auto-address in window.	*/
 boolean AutoDepend;			/* TRUE if auto-dependency in window.	*/
 AN_Id adra2;				/* AN_Id or register A part of adf2.	*/
 RegId regfa2;				/* RegId of register A part of adf2.	*/
#endif

#ifdef W32200
 AutoAddress = IsAutoAddress(pf,pf);		/* Check for autoaddressing. */
 AutoDepend = (AutoAddress && IsAutoDependency(adf0,adf1,adf2,adf3,NULL)) ?
		TRUE : FALSE;
#endif
/************************************************************************/
/*									*/
/* 100 -	If adf1 not immediate and adf2 is immediate and opcode	*/
/*	is commutative, flip adf1 and adf2.				*/
/*									*/
/************************************************************************/

 if(IsAdImmediate(adf1))			/* If first one is immediate */
	goto END100;				/* forget it.	*/
 if(!IsAdImmediate(adf2))			/* If second one is not	*/
	goto END100;				/* immediate, forget it. */
 switch(opf)					/* Is it commutative?	*/
	{default:				/* Here if no.	*/
		goto END100;
	 case G_ADD3:				/* These are commutative. */
	 case G_AND3:
	 case G_MUL3:
	 case G_OR3:
	 case G_XOR3:
		endcase;
	} /* END OF switch(opf) */
 if(Pskip(PEEP100))				/* Do we want to do it?	*/
	goto END100;				/* No.	*/
 peepchange("Reorder operands of commutative operators",peeppass,100);
 PutTxOperandAd(pf,SECOND_SRC,adf2);
 PutTxOperandType(pf,SECOND_SRC,tyf2);
 PutTxOperandAd(pf,THIRD_SRC,adf1);
 PutTxOperandType(pf,THIRD_SRC,tyf1);
 return(TRUE);
END100:
/************************************************************************/
/*									*/
/* 101 - folding:	eliminate dead code				*/
/*									*/
/*		G_OP src1 src2 src3 dst		->	[deleted]	*/
/*									*/
/*			where dst is dead				*/
/*									*/
/************************************************************************/

 if(optab[opf].oflags & CBR)		/* Not valid for conditional	  */
	goto END101;			/* jumps, or any instruction that */
					/* does not have a valid dst addr.*/
 if(!IsDeadAd(adf3,pf))			/* dst is dead */
	goto END101;
 if(setslivecc(pf))			/* sets no live conditional codes */
	goto END101;
 if(!IsAdSafe(adf0) || IsTxOperandVol(pf,0))		/* operands are not volatile */
	goto END101;
 if(!IsAdSafe(adf1) || IsTxOperandVol(pf,1))
	goto END101;
 if(!IsAdSafe(adf2) || IsTxOperandVol(pf,2))
	goto END101;
 if(!IsAdSafe(adf3) || IsTxOperandVol(pf,3))
	goto END101;

#ifdef W32200
					/* Next four tests are to ensure */
					/* that if any auto-addressing is */
					/* going on, that it is still done*/
					/* if the registers involved are */
					/* live. Order of tests determined */
					/* by guess of decreasing liklihood */
					/* of test failure (so that we do */
					/* the least number of tests for */
					/* cases where we are going to	*/
					/* fail).	*/
 if(IsAdAutoAddress(adf2) && !IsDeadAd(GetAdCPUReg(GetAdRegA(adf2)),pf))
	goto END101;
 if(IsAdAutoAddress(adf3) && !IsDeadAd(GetAdCPUReg(GetAdRegA(adf3)),pf))
	goto END101;
 if(IsAdAutoAddress(adf1) && !IsDeadAd(GetAdCPUReg(GetAdRegA(adf1)),pf))
	goto END101;
 if(IsAdAutoAddress(adf0) && !IsDeadAd(GetAdCPUReg(GetAdRegA(adf0)),pf))
	goto END101;
#endif

 if(Pskip(PEEP101))
	goto END101;
 peepchange("eliminate dead code",peeppass,101);
 ldelin2(pf);
 DelTxNode(pf);
 ndisc += 1;
 return(TRUE) ;
END101:
/************************************************************************/
/*									*/
/* 102 - removal of useless arithmetic					*/
/*	Since dead code has been removed, don't need to check		*/
/*	whether dst is alive. If the condition codes are alive,		*/
/*	result indicators must be preserved.				*/
/*	If auto-addressing is going on, we must be sure that it still	*/
/*	goes on if the concerned registers are live.			*/
/*									*/
/************************************************************************/

/* types of changes */
#define UA_MOV	3	/* change to move */
#define UA_MOVZ	4	/* chg to move  zero to */
#define UA_MCOM 5	/* chg to move complemented */
#define UA_MNEG 6	/* chg to move negative */


 if(!IsOpGeneric(opf))			/* Is a generic opcode? */
	goto END102;			/* No.	*/
 if(IsOpGMove(opf))			/* We will handle generic moves */
	goto END102;			/* in w103.	*/
 if(!IsAdImmediate(adf1))		/* Is 1st operand immediate? */
	goto END102;			/* No.	*/
 if(!IsAdNumber(adf1))			/* Is expression constant? */
	goto END102;			/* No.	*/
   				 	/* Yes, switch on the first operand. */
					/* This decides what type of */
 					/* change will be done. */
 switch(GetAdNumber(Tnone,adf1))
	{case 0:
		if(optab[opf].oflags & IDENT0)	/* Instructions marked IDENT0 */
			chgflg = UA_MOV;	/* produce adf2 if adf1	*/
						/* is a zero; e.g., addw &0,..*/
		else if((optab[opf].oflags & ZERO0) /* Instr. marked ZERO0 */
						/* produce a zero if adf1 */
						/* is a zero; e.g., mulw &0,..*/
				&& IsAdSafe(adf2) 
				&& !IsTxOperandVol(pf,2))
			{
#ifdef W32200
			 if(IsAdAutoAddress(adf2))	/* If auto-address, */
				{if(AutoDepend)	/* If AutoDependency,	*/
					goto END102;	/* no change.	*/
				 regfa2 = GetAdRegA(adf2);
				 adra2 = GetAdCPUReg(regfa2);
				 if(!IsDeadAd(adra2,pf))	/*If register */
							/* of adf2 is live, */
					goto END102;	/* no change.	*/
				}
#endif
			 chgflg = UA_MOVZ;	/* Change to MoveZero.	*/
			}
		else
			goto END102;
		endcase;
	 case 1:
	 	if(optab[opf].oflags & IDENT1)	/* Instructions marked IDENT1 */
			chgflg = UA_MOV;	/* produce adf2 if adf1 is a */
						/* one; e.g., mulw &1, ... */
		else
			goto END102;
		endcase;
	 case -1:
		switch(opf)
			{case G_AND3:
				chgflg = UA_MOV;
				endcase;
			 case G_XOR3:
				chgflg = UA_MCOM;
				endcase;
			 case G_MUL3:
				chgflg = UA_MNEG;
				endcase;
			 case G_DIV3:
				if(IsSigned(tyf1) && IsSigned(tyf2))
					chgflg = UA_MNEG;
				endcase;
			 default:		/* Anything else:	*/
				goto END102;	/* no change.	*/
			} /* END OF switch(opf) */
		endcase;
	 default:
		goto END102;			/* No change.	*/
	} /* END OF switch(GetAdNumber(Tnone,adf1)) */
 if(Pskip(PEEP102))
	goto END102;
						/* Do the change.	*/
 peepchange("remove useless arithmetic",peeppass,102);
 switch(chgflg) 
	{case UA_MOV:
		PutTxOpCodeX(pf,G_MOV);		/* Replace op-code.	*/
		PutTxOperandAd(pf,SECOND_SRC,i0);
		PutTxOperandType(pf,SECOND_SRC,Tword);
		endcase;
	 case UA_MOVZ:
		PutTxOpCodeX(pf,G_MOV);		/* Replace op-code.	*/
		PutTxOperandAd(pf,THIRD_SRC,i0);
		PutTxOperandType(pf,THIRD_SRC,Tword);
		PutTxOperandAd(pf,SECOND_SRC,i0);
		PutTxOperandType(pf,SECOND_SRC,Tword);
		endcase;
	 case UA_MCOM:
		PutTxOpCodeX(pf,G_MCOM);	/* Replace op-code. */
		PutTxOperandAd(pf,SECOND_SRC,i0);
		PutTxOperandType(pf,SECOND_SRC,Tword);
		endcase;
	 case UA_MNEG:
		PutTxOpCodeX(pf,G_MNEG);	/* Replace op-code. */
		PutTxOperandAd(pf,SECOND_SRC,i0);
		PutTxOperandType(pf,SECOND_SRC,Tword);
		endcase;
	 default:
		goto END102;
	} /* END OF switch(chgflg) */
 return(TRUE);

 END102:
/************************************************************************/
/*									*/
/* 103 - discard useless G_MOV's 					*/
/*	Change those where only condition codes are live to compares	*/
/*	against zero.							*/
/*									*/
/*  G_MOV  &0  &0  O1  O1	-> [deleted]				*/
/*				or					*/
/*	G_MOV	&0  &0  A A	->	G_CMP	&0  &0  A  &0		*/
/*									*/
/* The condition codes must not be alive following G_MOV if		*/
/* deletion is to take place.						*/
/* Note this will pick up code such as 					*/
/*		G_MULW3 &1,%r0,%r0 -> G_MOV %r0,%r0			*/
/*									*/
/************************************************************************/

 if(opf != G_MOV)				/* OpCode must be G_MOV. */
	goto END103;
 if((tyf3 != Tword) && (tyf3 != Tuword))	/* Destination  must be word. */
	goto END103;
 if(adf2 != adf3)				/* Operands must be the same. */
	goto END103;
 if(tyf2 != tyf3)				/* Types must be the same. */
	goto END103;
 if(!IsAdSafe(adf2) || IsTxOperandVol(pf,2))	/* operands are not volatile */
	goto END103;
 if(!IsAdSafe(adf3) || IsTxOperandVol(pf,3))
	goto END103;

#ifdef W32200
 if(AutoAddress)				/* Must have no auto-	*/
	goto END103;				/* addressing.	*/
#endif

 if(Pskip(PEEP103))
	goto END103;
 if(setslivecc(pf)) 				/* Change to test if sets */
						/* live condition codes. */
	{peepchange("change move to test when only CCs are needed",peeppass,103);
	 PutTxOpCodeX(pf,G_CMP);		/* Change to compare.	*/
	 PutTxOperandAd(pf,DESTINATION,i0);	/* Remove unused destination. */
	 PutTxOperandType(pf,DESTINATION,Tword);
	 PutTxOperandAd(pf,SECOND_SRC,i0);	/* This should be i0,	*/
	 PutTxOperandType(pf,SECOND_SRC,Tword);	/* but be careful.	*/
	}
 else
	{peepchange("discard useless moves",peeppass,103); /*Do the optimization.*/
	 ldelin2(pf);				/* Adjust line numbers.	*/
	 DelTxNode(pf);				/* Delete useless node.	*/
	 ndisc += 1;				/* Increment number discarded.*/
	}
 return(TRUE);
END103:
/************************************************************************/
/*									*/
/* 104 - change multiply, divide, or modulo by power of 2		*/
/*									*/
/*	G_MUL3   &0  &2^n  O1  O2  ->    G_LLS3  &0  &n      O1  O2	*/
/*	G_DIV3   &0  &2^n  O1  O2  ->    G_LRS3  &0  &n      O1  O2	*/
/*	G_MOD3	 &0  &2^n  O1  O2  ->    G_AND   &0  &2^n-1  O1  O2	*/
/*									*/
/*			where O1 and O2 are unsigned words		*/
/*									*/
/*	G_MUL3   &0  &2^n  O1  O2  ->    G_ARS3  &0  &n      O1  O2	*/
/*									*/
/*			where O1 and O2 and signed words		*/
/*									*/
/************************************************************************/

 switch(opf)
	{case G_MUL3:
		if((tyf2 != Tword) && (tyf2 != Tuword))
			goto END104;
		endcase;
	 case G_DIV3:
	 case G_MOD3:
		if(tyf2 != Tuword)		/* Can't do correctly */
						/* on signed quantities */
						/* with a single instr. */
			goto END104;
		endcase;
	 default:
		goto END104;
	} /* END OF switch(opf) */
 if(tyf2 != tyf3)
	goto END104;
 if((b = getbit(adf1) ) <= 0)			/* it is a power of 2 */
	goto END104;
 if(Pskip(PEEP104))
	goto END104;
 switch(opf)
	{case G_MUL3:
		peepchange("change mult by power of 2 to shift",peeppass,104);
		if(tyf2 == Tword)
			{PutTxOpCodeX(pf,G_ALS3);	/* signed */
			}
		else
			{PutTxOpCodeX(pf,G_LLS3);	/* unsigned */
			}
		PutTxOperandAd(pf,SECOND_SRC,GetAdAddToKey(Tword,i0,b));
		endcase;
	 case G_DIV3:				/* unsigned only */
		peepchange("change div by power of 2 to shift",peeppass,104);
		PutTxOpCodeX(pf,G_LRS3);	/*  Replace op-code. */
		PutTxOperandAd(pf,SECOND_SRC,GetAdAddToKey(Tword,i0,b));
		endcase;
	 case G_MOD3:				/* unsigned only */
		peepchange("change mod by power of 2 to mask",peeppass,104);
		PutTxOpCodeX(pf,G_AND3);	/* Replace op-code. */
		PutTxOperandAd(pf,SECOND_SRC,GetAdAddToKey(tyf1,adf1,-1));
		endcase;
	 default:
		goto END104;
	} /* END OF switch(opf) */
 return(TRUE);

END104:
/************************************************************************/
/*									*/
/* 107 -	Arithmetic Folding.					*/
/*									*/
/*	G_ADD3	i0  &A  &B  DEST  ->  G_MOV	i0  i0  &[B+A]  DEST	*/
/*									*/
/*	G_SUB3	i0  &A  &B  DEST  ->  G_MOV	i0  i0  &[B-A]  DEST	*/
/*									*/
/*	G_MUL3	i0  &A  &B  DEST  ->  G_MOV	i0  i0  &[B*A]  DEST	*/
/*									*/
/*	G_DIV3	i0  &A  &B  DEST  ->  G_MOV	i0  i0  &[B/A]  DEST	*/
/*		(A != 0)						*/
/*									*/
/*	G_CMP	i0  &A  &B  i0    ->  G_CMP	i0  i0  &[B-A]  i0	*/
/*									*/
/************************************************************************/

 if(!IsAdImmediate(adf1))			/* First operand must	*/
	goto END107;				/* be immediate	*/
 if(!IsAdNumber(adf1))				/* constant.	*/
	goto END107;
 if(!IsAdImmediate(adf2))			/* Second operand must	*/
	goto END107;				/* be immediate	*/
 if(!IsAdNumber(adf2))				/* constant.	*/
	goto END107;
 if(!IsDeadAd(C,pf))				/* C condition code, */
						/* indicating unsigned branch,*/
	goto END107;				/* must not be live. */
 if(Pskip(PEEP107))				/* Do we want to do this. */
	goto END107;				/* No.	*/
 value1 = ExtendItem(tyf1,GetAdNumber(tyf1,adf1));	/* Be sure items are */
 value2 = ExtendItem(tyf2,GetAdNumber(tyf2,adf2));	/* corrected for */
						/* their data types.	*/
 switch(opf)					/* What we do depends on code.*/
	{case G_ADD3:
		result = value2 + value1;
		new_op = G_MOV;
		endcase;
	 case G_CMP:
		if(adf1  == i0)			/* If "second" operand is */
			goto END107;		/* already 0, skip it. */
		result = value2 - value1;
		new_op = G_CMP;
		endcase;
	 case G_SUB3:
		result = value2 - value1;
		new_op = G_MOV;
		endcase;
	 case G_MUL3:
		result = value2 * value1;
		new_op = G_MOV;
		endcase;
	 case G_DIV3:
		if(value1 == 0)
			goto END107;
		if(tyf1 == Tuword || tyf2 == Tuword)
			/*
			 * To avoid possible problems when pf
			 * is of the form, say,
			 *	G_DIV	&-n,{uword}&m,X 
			 * which can be a result of transformation
			 * from the original or a result of previous
			 * optimizations.
			 */
			goto END107;
		result = value2 / value1;
		new_op = G_MOV;
		endcase;
	 default:
		goto END107;
	} /* END OF switch(opf) */
 peepchange("Constant Folding",peeppass,107);
 PutTxOpCodeX(pf,new_op);			/* Change to G_MOV.	*/
 PutTxOperandAd(pf,SECOND_SRC,i0);		/* Delete "second" source. */
 PutTxOperandType(pf,SECOND_SRC,Tword);
 PutTxOperandAd(pf,THIRD_SRC,GetAdAddToKey(tyf3,i0,result));	/* Fix "third"*/
 return(TRUE);
END107:

#ifdef W32200
/************************************************************************/
/*									*/
/* 108 -	Convert PushAddress to move ,(%sp)+ on 32200 if in	*/
/*	speed mode.							*/
/*									*/
/************************************************************************/

 if(cpu_chip != we32200)			/* Is it a 32200 chip?	*/
	goto END108;				/* No: cannot do it.	*/
 if(optmode == OSIZE)				/* Are we in size mode?*/
	goto END108;				/* No: don't wish to do it. */
 if(	(opf != G_PUSHA)			/* Is it a PUSHA?	*/
    &&	(opf != G_PUSH))			/* or a PUSH?	*/
	goto END108;				/* No: does not apply.	*/
 if(Pskip(PEEP108))				/* Do we want to do it?	*/
	goto END108;				/* No: skip it.	*/
 peepchange("Change PUSH's to MOVE's.",peeppass,108);
 if(opf == G_PUSHA)
	PutTxOpCodeX(pf,G_MOVA);		/* Change op-code.	*/
 else
	PutTxOpCodeX(pf,G_MOV);			/* Change op-code.	*/
 an_id = GetAdPostIncr(tyf2,"",CSP);		/* Get stack post increment. */
 PutTxOperandAd(pf,DESTINATION,an_id);		/* Put address in instr. */
 PutTxOperandType(pf,DESTINATION,tyf2);		/* ... and type.	*/
 return(TRUE);					/* We did it.	*/
END108:
#endif
/************************************************************************/
/*									*/
/* 109 -	Replace multiplys by shifts and adds when it seems	*/
/*	to be faster.							*/
/*									*/
/************************************************************************/

 if(optmode == OSIZE)				/* Makes it bigger.	*/
	goto END109;
 if(opf != G_MUL3)				/* Must be a multiply.	*/
	goto END109;
 if(!IsAdImmediate(adf1))			/* Must be by an	*/
	goto END109;				/* immediate constant.	*/
 if(!IsAdNumber(adf1))
	goto END109;
 if(IsAdUses(adf2,adf3))			/* NO DYADICs or if src	*/
	goto END109;				/* is affected by dest. */
 if(tyf2 != tyf3)				/* NO Conversions.	*/
	goto END109;
 if(tyf3 != Tword && tyf3 != Tuword)
	goto END109;
 if(!IsAdSafe(adf2) || IsTxOperandVol(pf,2))	/* Must not be volatile. */
	goto END109;
 if(!IsAdSafe(adf3) || IsTxOperandVol(pf,3))
	goto END109;

#ifdef W32200
 if(AutoAddress)				/* Cannot tolerate auto- */
	goto END109;				/* addressing.	*/
#endif

 value.UI[0] = GetAdNumber(tyf1,adf1);		/* Compute number of shifts. */
 if(value.UI[0] == 0)				/* If multiply by zero,	*/
	goto END109;				/* someone else will do it. */
 result  = CountOneBits(value.UC[0]);
 result += CountOneBits(value.UC[1]);
 result += CountOneBits(value.UC[2]);
 result += CountOneBits(value.UC[3]);
 if(!IsAdCPUReg(adf3))				/* If result not a CPU	*/
	result += 1;				/* register, penalty.	*/
 if(optmode != OSPEED)				/* If not speed mode,	*/
	result += 1;				/* do a little less.	*/
 if(result > LIMIT109)				/* If too many shifts,	*/
	goto END109;				/* forget it.	*/
 if(Pskip(PEEP109))				/* Do we want to do this? */
	goto END109;				/* No.	*/
 peepchange("Change multiply to shift and add",peeppass,109);
 PutTxOpCodeX(pf,G_MOV);			/* Change Multiply to clear. */
 PutTxOperandAd(pf,SECOND_SRC,i0);		/* Get rid of multiplyer. */
 PutTxOperandType(pf,SECOND_SRC,Tword);
 PutTxOperandAd(pf,THIRD_SRC,i0);		/* Get rid of multiplicand. */
 PutTxOperandType(pf,THIRD_SRC,Tword);
 LastNode = pf;					/* Put in the shifts and adds.*/
 ShiftAmount = -1;
 for(index = 31; index >= 0; index--)
	{if(get_bit(value.UI,index) != 0)
		{if(ShiftAmount > 0)
			{LastNode = MakeTxNodeAfter(LastNode,G_LLS3);
			 an_id = GetAdAddToKey(Tword,i0,ShiftAmount);
			 PutTxOperandAd(LastNode,SECOND_SRC,an_id);
			 PutTxOperandType(LastNode,SECOND_SRC,Tword);
			 PutTxOperandAd(LastNode,THIRD_SRC,adf3);
			 PutTxOperandType(LastNode,THIRD_SRC,tyf3);
			 PutTxOperandAd(LastNode,DESTINATION,adf3);
			 PutTxOperandType(LastNode,DESTINATION,tyf3);
			}
		 LastNode = MakeTxNodeAfter(LastNode,G_ADD3);
		 PutTxOperandAd(LastNode,SECOND_SRC,adf2);
		 PutTxOperandType(LastNode,SECOND_SRC,tyf2);
		 PutTxOperandAd(LastNode,THIRD_SRC,adf3);
		 PutTxOperandType(LastNode,THIRD_SRC,tyf3);
		 PutTxOperandAd(LastNode,DESTINATION,adf3);
		 PutTxOperandType(LastNode,DESTINATION,tyf3);
		 ShiftAmount = 1;
		} /* END OF if(get_bit(index,value.UI) != 0) */
	 else
		if(ShiftAmount >= 0)
			ShiftAmount += 1;
	} /* END OF for(index = 31; index >= 0; index--_ */
 if(ShiftAmount > 1)
	{LastNode = MakeTxNodeAfter(LastNode,G_LLS3);
	 an_id = GetAdAddToKey(Tword,i0,(ShiftAmount - 1));
	 PutTxOperandAd(LastNode,SECOND_SRC,an_id);
	 PutTxOperandType(LastNode,SECOND_SRC,Tword);
	 PutTxOperandAd(LastNode,THIRD_SRC,adf3);
	 PutTxOperandType(LastNode,THIRD_SRC,tyf3);
	 PutTxOperandAd(LastNode,DESTINATION,adf3);
	 PutTxOperandType(LastNode,DESTINATION,tyf3);
	}
 return(TRUE);					/* We did a change.	*/
END109:
/* end of one-instruction peephole optimizations for passes 1 and 2 */
 if(peeppass <= 2)
	return( FALSE );
/************************************************************************/
/*									*/
/* 111 - Shift by one bit is more efficently done as an add		*/
/*									*/
/*	G_LLS3   &0  &1  O1  O2	   ->	G_ADD3 &0  O1  O1  O2		*/
/*                 for TWORDs only					*/
/*									*/
/************************************************************************/

 if(opf != G_LLS3)
	goto END111;
 if(tyf2 != Tword)
	goto END111;
 if(tyf2 != tyf3)
	goto END111;
 if(adf1 != i1)
	goto END111;
 if(!IsAdSafe(adf2) || IsTxOperandVol(pf,2))	/* is safe from volatility */
	goto END111;

#ifdef W32200
 if(IsAdAutoAddress(adf2))			/* O1 must not be auto-address*/
	goto END111;
#endif

 if(setslivecc(pf))				/* Must not set cond codes */
						/* because shift does not */
						/* set them the same as */
	goto END111;				/* add does. */
 if(Pskip(PEEP111))
	goto END111;
						/* Do the change.	*/
 peepchange("change shift by one to add",peeppass,111);
 PutTxOpCodeX(pf,G_ADD3);			/* Replace op-code.	*/
 PutTxOperandAd(pf,SECOND_SRC,adf2);
 PutTxOperandType(pf,SECOND_SRC,tyf2);
 return(TRUE);
END111:
 return(FALSE);
}
/* end of w1opt */
