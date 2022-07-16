/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)newoptim:m32/w2opt.c	1.11"

/****************************************************************/
/*				w2opt.c				*/
/*								*/
/*		Two instruction peephole window.		*/
/*								*/
/*	This module contains the code that improves two-	*/
/*	instruction equences. The general scheme is to put	*/
/*	those improvements first which result in removing code,	*/
/*	followed by the others.					*/
/*								*/
/****************************************************************/

/****************************************************************/
/*								*/
/*		Some general caveats (learned the hard way):	*/
/*								*/
/*	1. When instructions get interchanged, we must take	*/
/*	care that we don't alter the condition codes that would	*/
/*	have resulted when executing the instructions in their	*/
/*	original order.						*/
/*								*/
/*	2. We can't move adds to %sp, since we may move them	*/
/*	after the place which refers to the newly allocated	*/
/*	space on the stack.					*/
/*								*/
/****************************************************************/

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
#include	"ALNodeType.h"
#include	"FNodeTypes.h"
#include	"FNodeDefs.h"
#include	"optab.h"
#include	"optim.h"
#include	"optutil.h"
#include	"Target.h"

#define	SOURCE2		2
#define DESTINATION	3
					/* types of changes for 22x peepholes */
#define	AA_NOP		1		/* C unchanged */
#define AA_COPY		2		/* C -> A */
#define AA_DISP		3		/* C -> expr+n(RA),n(RA) */
#define AA_NUM_DISP	4		/* C -> expr+n(RA),n(RA) */
#define AA_PRE_INCR	5		/* C -> +(RA) */
#define AA_PRE_DECR	6		/* C -> -(RA) */
#define AA_POST_INCR	7		/* C -> (RA)+ */
#define AA_POST_DECR	8		/* C -> (RA)- */
#define AA_PUSHA	9		/* PUSH C -> PUSHA n(RA) */
#define AA_INDEX_REG	10		/* C -> expr(RC1st,RA),expr(RA,RC2nd) */
					/* C -> RC1st[RA],RC2nd[RA] */
#define AA_INDIRECT	11		/* C -> indirect(A) */
	boolean					/* TRUE if changes made */
w2opt(pf,pl,peeppass)
TN_Id pf;			/* First instruction node of window.	*/
TN_Id pl;			/* Second instruction node.	*/
unsigned int peeppass;		/* Peephole pass number.	*/

{extern AN_Id A;		/* AN_Id of A Condition Code.	*/
 extern AN_Id C;		/* AN_Id of C Condition Code.	*/
 extern AN_Id N;		/* AN_Id of N Condition Code.	*/
 extern AN_Id V;		/* AV_Id of V Condition Code.	*/
 extern AN_Id Z;		/* AZ_Id of Z Condition Code.	*/
 extern long int AddressDelta();/* Amount address changes its register. */
 static unsigned int CC_A;	/* Bit for A condition code.	*/
 static unsigned int CC_C;	/* Bit for C condition code.	*/
 static unsigned int CC_N;	/* Bit for N condition code.	*/
 static unsigned int CC_V;	/* Bit for V condition code.	*/
 static unsigned int CC_X;	/* Bit for V condition code.	*/
 static unsigned int CC_Z;	/* Bit for V condition code.	*/
 static FN_Id CurFuncId;	/*Id of function we think is current function.*/
 extern long int EffectiveDisp(); /* Returns effective address. */
 extern FN_Id FuncId;		/* Id of current function. */
 extern boolean IsInstrDeleteAllowed(); /* TRUE if allowed for peep22x.*/ 
 extern boolean IsInstrMoveAllowed(); /* TRUE if allowed for peep22x.*/ 
 extern boolean IsInstrDeleteOrMoveAllowed(); /* TRUE if allowed for peep22x.*/ 
 extern boolean IsOpUses();	/* TRUE if opcode uses address. */
 extern boolean IsSameEffAdd();	/* TRUE if addresses the same.	*/
 extern unsigned int Max_Ops;	/* Maximum operands in an instruction.	*/
 extern int TySize();		/* Returns size in bytes of a type. */
 AN_Id adf0 = GetTxOperandAd(pf,0);
 AN_Id adf1 = GetTxOperandAd(pf,1);
 AN_Id adf2 = GetTxOperandAd(pf,2);
 AN_Id adf3 = GetTxOperandAd(pf,3);
 AN_Id adl0 = GetTxOperandAd(pl,0);
 AN_Id adl1 = GetTxOperandAd(pl,1);
 AN_Id adl2 = GetTxOperandAd(pl,2);
 AN_Id adl3 = GetTxOperandAd(pl,3);
 int bi;			/* Value of immediate constant for 207.	*/
 boolean f;
 extern void fatal();
 extern AN_Id i0;		/* Immediate 0 address id.	*/
 extern boolean islivecc();	/* TRUE if condition codes live.	*/
 extern void ldelin();		/* Delete line number. */
 extern void lexchin();
 static unsigned long int live_ccs;	/* Live condition codes.	*/
 extern void lmrgin3();
 extern unsigned int ndisc;	/* Number of instructions discarded.	*/
 register unsigned int opf 
	= GetTxOpCodeX(pf);	/* op code number of first inst. */
 register unsigned int opl 
	= GetTxOpCodeX(pl);	/* op code number of second inst. */
 extern struct opent optab[];	/* opcode table.	*/
 register unsigned int operand;	/* Operand counter.	*/
 extern void peepchange();
 static unsigned int set_ccs;	/* Condition codes set.	*/
 OperandType temp;	 	/* General Operand Type temporary.	*/
 AN_Id tmpaddr;			/* General address node identifier.	*/
 OperandType tyf0 = GetTxOperandType(pf,0); 
 OperandType tyf1 = GetTxOperandType(pf,1); 
 OperandType tyf2 = GetTxOperandType(pf,2); 
 OperandType tyf3 = GetTxOperandType(pf,3); 
 int szf2 = TySize(tyf2);
 int szf3 = TySize(tyf3);
 OperandType tyl0 = GetTxOperandType(pl,0); 
 OperandType tyl1 = GetTxOperandType(pl,1); 
 OperandType tyl2 = GetTxOperandType(pl,2); 
 OperandType tyl3 = GetTxOperandType(pl,3); 
 int szl1 = TySize(tyl1);
 int szl2 = TySize(tyl2);
 int szl3 = TySize(tyl3);
 int szwd = TySize(Tword);

#ifdef W32200
 boolean AutoAddress;		/* TRUE if auto-address in window.	*/
 boolean AutoDepend;		/* True if auto-dependencies.	*/
 extern boolean IsAutoAddress();	/* TRUE if auto-address in window. */
 extern boolean IsAutoDependency();	/* TRUE if auto-dependencies.	*/
 extern AN_Id X;		/* AX_Id of X Condition Code.	*/
#endif

#ifdef W32200
 AutoAddress = IsAutoAddress(pf,pl);		/* Check for autoaddressing. */
 AutoDepend = (AutoAddress
		&& IsAutoDependency(	adf0,adf1,adf2,adf3,
					adl0,adl1,adl2,adl3,NULL)) ?
			TRUE : FALSE;
#endif

/*	NOTE:	in the following tests, the order of the tests has been	*/
/*	determined, in part, by the measured frequencies in a test file	*/
/*	believed to be typical. Changes in the order should be made	*/
/*	only if based on measurements on a typical test file using the	*/
/*	lprof profiliing technique.					*/

/************************************************************************/
/*									*/
/* 200 - folding:	merge moves back into generics			*/
/*									*/
/*		G_op	A  B  C  D	->	G_op	A  B  C  F	*/
/*		G_move &0 &0  E  F					*/
/*									*/
/*			where address of D equals address of E,		*/
/*			E is dead following the move,			*/
/*			F does not use D in any way, and		*/
/*			condition codes of the move are not needed.	*/
/*									*/
/************************************************************************/

 if(!IsOpGMove(opl))			/* second opcode is a generic move */
	goto END200;
 if(adf3 != adl2) 			/* ad D == ad E */
	goto END200;
 if(!IsDeadAd(adl2,pl))			/* E is dead, also implies that
					it and D are safe from volatility */
	goto END200;
 if(!IsOpGeneric(opf))			/* first opcode is a generic */
	goto END200;
 if(IsOpDstSrc(opf))			/* D in G_op is a pure destination */
	goto END200;
 if(!IsAdCPUReg(adl3) && !IsAdCPUReg(adf3) && (szf3 < szl3))
					/* if F and D are memory, require 
					   sz(D) >= sz(F) in order to not 
					   loose truncation by D*/
	goto END200;
 if(IsAdCPUReg(adl3) && !IsAdCPUReg(adf3) && (szf3 != szwd))
					/* if F is a register and D is memory,
					   require sz(D) == sz(int) in order
					   to not loose truncation by D */
	goto END200;
 if(!IsAdCPUReg(adl3) && (szl2 < szl3))
					/* if F is memory, require
					   sz(E) >= sz(F) in order to not
					   loose truncation by E */
	goto END200;
 if(IsAdCPUReg(adl3) && (szl2 < szwd))
					/* if F is a register, require
					   sz(E) == sz(int) in order to not
					   loose truncation by E */
	goto END200;
 if((IsFP(tyf3) || IsFP(tyl2))
    && !((szf3 == szl2) && (szl2 >= szl3))
   )
					/* types okay if D or E is fp */
	goto END200;
 if(!notaffected(adl3,adf3))		/* F does not use D in any way */
	goto END200;

#ifdef W32200
 if(AutoDepend)					/* If autodependency,	*/
	goto END200;				/* we don't dare.	*/
#endif

 if(setslivecc(pl))			/* condition codes of second 
					instruction not needed */
	goto END200;
 if(!legalgen((unsigned short) opf, tyf0, GetAdMode(adf0),
		      tyf1, GetAdMode(adf1),
		      tyf2, GetAdMode(adf2),
		      tyl3, GetAdMode(adl3))
   )					 /* new instruction is legal */
	goto END200;
 if(Pskip(PEEP200))
	goto END200;

	{peepchange( "merge moves back" ,peeppass,200);
	 PutTxOperandAd(pf,DESTINATION,adl3);
	 PutTxOperandType(pf,DESTINATION,tyl3);
	 lmrgin3(pf,pl,pf);
	 DelTxNode(pl);
	 ndisc += 1;				/* Update number deleted. */
	 return(TRUE);
	}
 END200:
/************************************************************************/
/*									*/
/* 202 - folding:	remove unnecessary moves before generics	*/
/*									*/
/*		G_move &0 &0  A  B					*/
/*		G_op   	C  D  E  F	->	G_op   C  D  A  F	*/
/*									*/
/*					or				*/
/*									*/
/*		G_move &0 &0  A  B					*/
/*		G_op   	C  E  D  F	->	G_op   C  A  D  F	*/
/*									*/
/*			where address of B equals address of E,		*/
/*			C and D do not use B in any way,		*/
/*			either F is same as B or F does not use B in	*/ 
/*			any way and B is dead after G_op, and the 	*/
/*			condition codes of the G_move are not needed 	*/
/*									*/
/*	Additional requirements imposed to support the WE32200:		*/
/*									*/
/*	B must not be an auto-address mode.				*/
/*	For first case:							*/
/*		A must not be an auto-address mode that uses C.		*/
/*		A must not be an auto-address mode that uses D.		*/
/*		C must not be an auto-address mode that uses A.		*/
/*		D must not be an auto-address mode that uses A.		*/
/*	    If A is auto-address mode,					*/
/*		C must not use register of A.				*/
/*		D must not use register of A.				*/
/*									*/
/*	For the second case:						*/
/*		A must not be an auto-address mode that uses C.		*/
/*		C must not be an auto-address mode that uses A.		*/
/*	    If A is auto-address mode,					*/
/*		C must not use register of A.				*/
/*									*/
/************************************************************************/

 if(!IsOpGMove(opf))			/* first opcode is a gen move */
	goto END202;

#ifdef W32200
 if((IsAdAutoAddress(adf3))			/* If B is AutoAddress and */
		&& !IsDeadAd(GetAdCPUReg(GetAdRegA(adf3)),pf))	/* register */
	goto END202;				/* of B is live, we cannot do */
						/* this because we must	*/
						/* diddle the register.	*/
#endif
					/* flag indicating first case */
 f = (boolean) ((adf3 == adl2) && chktyp(pf,2,pl,2,&temp,&tmpaddr));
					/* B == E and types are okay */
 if(!f && !(adf3 == adl1 && chktyp(pf, 2, pl, 1, &temp, &tmpaddr))) 
	goto END202;
 if(!((adl3 == adf3) && ((szl3 >= szf3) || IsAdCPUReg(adl3)))
    && !(notaffected(adl3,adf3) && IsDeadAd(adf3, pl))
   )
					/* F is same as B or
				  	   F does not use B 
					   and B is dead after G_op */
	goto END202;

 if(!IsOpGeneric(opl))			/* second opcode is a generic */
	goto END202;
 if(opl == G_MOVA)			/* second opcode is not G_MOV */
	goto END202;
 if(opl == G_PUSHA)			/* second opcode is not G_PUSHA */
	goto END202;
 if(IsOpDstSrc(opl))			/* second opcode has pure dst */
	goto END202;

#ifdef W32200
 if(AutoDepend)					/* We don't trust these */
	goto END202;				/* if any auto dependencies. */
 if(f)						/* Must not have auto-	*/
	{if(IsAutoDependency(adl0,adl1,adf2,adl3,NULL))	/* dependencies	*/
		goto END202;			/* in result either.	*/
	}
 else
	{if(IsAutoDependency(adl0,adf2,adl2,adl3,NULL))
		goto END202;
	}
#endif

 if(!notaffected(adl0,adf3))		/* C does not use B */
	goto END202;
 if(!notaffected(GetTxOperandAd(pl,(unsigned int) ((f) ? 1 : 2)),adf3))
					/* D does not use B */
	goto END202;
 if(!legalgen((unsigned short) opl,tyl0,GetAdMode(adl0),	/* legal? */
		  (f?tyl1:temp),(f?GetAdMode(adl1):GetAdMode(tmpaddr)),
		  (f?temp:tyl2),(f?GetAdMode(tmpaddr):GetAdMode(adl2)),
		     tyl3,GetAdMode(adl3)))
	goto END202;
 if(setslivecc(pf))
	goto END202;
 if(Pskip(PEEP202))
	goto END202;
					/* okay to do optimization */
 peepchange( "merge moves forward" ,peeppass,202);
 PutTxOperandAd(pl,(unsigned int) ((f) ? 2 : 1),tmpaddr);
 PutTxOperandType(pl,(unsigned int) ((f) ? 2 : 1),temp);
 lmrgin3(pf,pl,pl);
 DelTxNode(pf);
 ndisc += 1;					/* Update number discarded. */
 return(TRUE);

 END202:
/************************************************************************/
/*									*/
/* 203 - folding:	eliminate compares against zero after condition */
/*			code setting operations.			*/
/*									*/
/*		G_OP   A  B  C  D	->	G_OP  A  B  C  D	*/
/*		G_CMP &0  &0 D &0					*/
/*									*/
/*					or				*/
/*									*/
/*		G_MOV  A  B  C  D	->	G_MOV  A  B  C  D	*/
/*		G_CMP &0  &0 C &0					*/
/*			(where G_MOV does no conversions)		*/
/*									*/
/*			where the conditions codes are set in the same	*/
/*			way as by a compare or the compare is dead	*/
/*									*/
/************************************************************************/

 if(!IsOpCompare(opl))				/* second opcode is a compare */
	goto END203;
 if(!IsOpGeneric(opf))				/* first opcode is a generic */
	goto END203;
 if(	!(	IsSameEffAdd(pf,3,pl,2)
	    &&	(tyf3 == tyl2)			/* pfD == plD */
	 )
    &&	!(	(opf == G_MOV)
	    &&	(tyf2 == tyf3)
	    &&	IsSameEffAdd(pf,2,pl,2)
	    &&	(tyf2 == tyl2)
	    &&	!IsAdUses(adl2,adf3)
	 )
   ) 						/* OR G_OP is move	*/ 
	goto END203;     			/* AND pfC == plD.	*/
 if(adl1 != i0) 				/* comparison to zero */
	goto END203;

 /*
  * If the first instruction sets a condition code differently
  * from the compare instruction and that condition code is used
  * subsequently, then we can't do this optimization.
  * For example,
  *	G_ADD3	X,Y,Z
  *	G_CMP	Z,&0
  *	jgu	label
  * the carry bit after the G_ADD instruction reflects the add
  * operation, but the carry bit set by G_CMP is determined by
  * 0 - Z.
  */
 GetTxLive(pl,&live_ccs,1);			/* (First word of live data.) */
						/* We assume condition codes */
						/* are all in first word. */
 if(CurFuncId != FuncId)			/* Starting new function? */
	{CurFuncId = FuncId;			/* Yes: re-compute stuff. */
	 if((math_chip == we32106) || (math_chip == we32206))
		{if(IsAdAddrIndex(A))
			CC_A = (1 << GetAdAddrIndex(A));
		 else
			CC_A = 0;
		}
	 if(IsAdAddrIndex(C))
	 	CC_C = (1 << GetAdAddrIndex(C));
	 else
		CC_C = 0;
	 if(IsAdAddrIndex(N))
	 	CC_N = (1 << GetAdAddrIndex(N));
	 else
		CC_N = 0;
	 if(IsAdAddrIndex(V))
	 	CC_V = (1 << GetAdAddrIndex(V));
	 else
		CC_V = 0;

#ifdef W32200
	 if(cpu_chip == we32200)
		{if(IsAdAddrIndex(X))
		 	CC_X = (1 << GetAdAddrIndex(X));
		 else
			CC_X = 0;
		}
#endif

	 if(IsAdAddrIndex(Z))
	 	CC_Z = (1 << GetAdAddrIndex(Z));
	 else
		CC_Z = 0;
	} /* END OF if(CurFuncId != FuncId) */
 live_ccs &= (CC_A | CC_C | CC_N | CC_V | CC_X | CC_Z);	/*Live condition codes.*/

 set_ccs = 0;
 switch(optab[opf].oflags & 		/* Find bits set differently from TST.*/
		(CCNZ00 | CCNZCV | CCNZ0V))
	{case CCNZ00:			/* logical and field instructions */
		endcase;
	 case CCNZCV:			/* additive arithmetic instructions */
		set_ccs = CC_C | CC_V;
		endcase;
	 case CCNZ0V:			/* non-additive arithmetic instructions */
		set_ccs = CC_V;
		endcase;
	 default:			/* Don't know: assume the worst. */
		set_ccs = CC_A | CC_C | CC_N | CC_V | CC_X | CC_Z;
		endcase;
	} /*  END OF switch(optab[opf].oflags & ...) */

 if((live_ccs & set_ccs) != 0)			/* Fail if live ones mis-set. */
	goto END203;

 if(IsOpMIS(opf))				/* Alas! this will not work */
					 	/* for mis instructions since */
						/* no mis instruction sets */
						/* the conditional flags like */
						/* the mis compare. */
	goto END203;
 if(IsOpMIS(opl))				/* Same for second instr */
	goto END203;
 if(!IsAdSafe(adl2) || IsTxOperandVol(pl,2))	/* The improvement prevents D */
						/* from being read, so it better
						   not be volatile */
	goto END203;
 if(Pskip(PEEP203))
	goto END203;
   						/* okay to do change */
 peepchange( "eliminate compares against zero" ,peeppass,203);
 lmrgin3(pf,pl,pf);
 DelTxNode(pl);
 ndisc += 1;					/* Update number discarded. */
 return(TRUE);

 END203:
/************************************************************************/
/*									*/
/* 204 - folding:	redundant moves					*/
/*									*/
/*		G_MOV &0 &0  A  B					*/
/*		G_MOV &0 &0  B  A	->	G_MOV &0 &0  A  B	*/
/*									*/
/*					or				*/
/*		G_MOV &0 &0  A  B					*/
/*		G_MOV &0 &0  A  B	->	G_MOV &0 &0  A  B	*/
/*									*/
/*			where A does not use B in any way in second case*/
/*			      A and B are of the same type		*/
/*									*/
/*		Cases apply also to MIS code				*/
/*									*/
/************************************************************************/

 if(!IsOpGMove(opf))			/* first instr is move */
	goto END204;

 f = (boolean) ((adf2 == adl3) && (tyf2 == tyl3)
 		&& (adf3 == adl2) && (tyf3 == tyl2)
	       );			/* flag for first case A==A and B==B */

 if(!f && !((adf2 == adl2) && (tyf2 == tyl2) &&	
	    (adf3 == adl3) && (tyf3 == tyl3)
	   )
   )					/* first case or */
					/* second case A==A and B==B */
	goto END204;
 if(!IsOpGMove(opl))			/* second instr is move */
	goto END204;
 if(tyf2 != tyf3)			/* type of A == type of B, this is a 
					   really weak test.  All of the 
					   possible data type combinations 
					   need to be thought out on size and 
					   format (a la mis instructions) for 
					   moves of the form:
						mov a c
						mov c b -> mov a b */
	goto END204;
 if(!IsAdSafe(adf2) || IsTxOperandVol(pf,2))	/* A must not be volatile */
	goto END204;
 if(!IsAdSafe(adf3) || IsTxOperandVol(pf,3))	/* B must not be volatile */
	goto END204;

#ifdef W32200
 if(AutoAddress)				/* If either of the	*/
	goto END204;				/* operands is auto-	*/
						/* address, skip this	*/
						/* because we must diddle */
						/* the registers in any case. */
#endif

 if(!f && !notaffected(adf2,adf3))	/* A does not use B any way 
					   in second case */
	goto END204;
 if(Pskip(PEEP204))
	goto END204;
					/* okay to change */
 peepchange( "merging redundant moves" ,peeppass,204);
 if (f) 
	{lmrgin3(pf,pl,pf);
	 DelTxNode(pl);
	}
 else
	{lmrgin3(pf,pl,pl);
	 DelTxNode(pf);
	}
 ndisc += 1;					/* Update number discarded. */
 return(TRUE);

 END204:
/************************************************************************/
/*									*/
/*	207 - Change move to add to prepare for other optimizations.	*/
/*									*/
/*	G_MOV	i0   i0  N1  N2  ->	G_ADD3	i0   S   N1  N1		*/
/*	G_ADD3	i0   S   N1  N1		G_SUB3	i0   S   N1  N2		*/
/*									*/
/*				or					*/
/*									*/
/*	G_MOV	i0   i0  N1  N2	->	G_SUB3	i0   S   N1  N1		*/
/*	G_SUB3	i0   S   N1  N1		G_ADD3	i0   S   N1  N2		*/
/*									*/
/************************************************************************/

 if(opf != G_MOV)				/* First operator must be */
	goto END207;				/* a G_MOV.	*/
 if(opl == G_ADD3)				/* Second operator must be */
	f = TRUE;				/* a generic add,	*/
 else if (opl == G_SUB3)			/* or a generic subtract. */
	f = FALSE;
 else
	goto END207;				/*Optimization not applicable.*/
 if(adf2 != adl2)				/* Establish operand pattern. */
	goto END207;
 if(adl2 != adl3)
	goto END207;
 if(adf2 == adf3)				/* (Strange if true.)	*/
	goto END207;
 if((tyf2 != Tword) && (tyf2 != Tuword))
	goto END207;
 if((tyf3 != Tword) && (tyf3 != Tuword))
	goto END207;
 if(!IsAdImmediate(adl1))			/* S must be immediate.	*/
	goto END207;
 if(!IsAdNumber(adl1))				/* S must be constant.	*/
	goto END207;

#ifdef W32200
 if(AutoAddress)				/* If autoaddressing going */
	goto END207;				/* on, skip it.	*/
#endif

 if(!IsAdCPUReg(adf2))				/* CPURegs only.	*/
	goto END207;
 if(!IsAdCPUReg(adf3))
	goto END207;
 if(setslivecc(pl))				/* Don't do if condition */
	goto END207;				/* codes are live.	*/
 if(Pskip(PEEP207))				/* Do we want to do this? */
	goto END207;				/* No.	*/
 bi = GetAdNumber(tyl1,adl1);			/* Get the increment.	*/
 if(bi > szwd)
	goto END207;
 if(bi < -szwd)
	goto END207;
 if(f)						/* Case 1?	*/
	{peepchange("change move to subtract",peeppass,207);
	 PutTxOpCodeX(pf,G_SUB3);		/* Change G_MOV to G_SUB3. */
	}
 else						/* Case 2.	*/
	{peepchange("change move to add",peeppass,207);
	 PutTxOpCodeX(pf,G_ADD3);		/* Change G_MOV to G_ADD3. */
	}
 PutTxOperandAd(pf,1,adl1);			/* Put in operand S.	*/
 PutTxOperandType(pf,1,tyl1);
 (void) MoveTxNodeAfter(pf,pl);			/* Flip them around.	*/
 lexchin(pf,pl);				/* We exchanged them.	*/
 return(TRUE);
END207:
/************************************************************************/
/*									*/
/* 209 -	Eliminate or defer MOVAW if possible.			*/
/*									*/
/*	MOVAW	S,NAQ		->	PUSHAW	S			*/
/*	PUSHW	NAQ							*/
/*	(NAQ Dead)							*/
/*									*/
/*				or					*/
/*									*/
/*	MOVAW	S,NAQ		->	PUSHAW	S			*/
/*	PUSHW	NAQ			MOVAW	S,NAQ			*/
/*	(NAQ live)							*/
/*									*/
/************************************************************************/

 if(opf != G_MOVA)				/* First op must be MOVAW. */
	goto END209;
 if(tyf2 != Tword)
	goto END209;
 if(opl != G_PUSH)				/* Second must be PUSHW. */
	goto END209;
 if(tyl2 != Tword)
	goto END209;
 if(!IsAdNAQ(adf3) && !IsAdSNAQ(adf3))		/* Destination of MOVAW must */
	goto END209;				/* a NAQ.	*/
 if(adf3 != adl2)				/* Destination of MOVAW must */
	goto END209;				/* be source of PUSHAW.	*/
 if(Pskip(PEEP209))				/* Do we want to do this? */
	goto END209;				/* No.	*/
 peepchange("Remove or defer MOVAW before PUSHW",peeppass,209);
 if(IsDeadAd(adf3,pl))				/* Is NAQ live or dead?	*/
	{lmrgin3(pf,pl,pf);			/* Adjust line numbers.	*/
	 PutTxOperandAd(pl,SOURCE2,adf2);	/* Fix PUSHW source operand. */
	 PutTxOpCodeX(pl,G_PUSHA);		/* Convert PUSHW to PUSHAW. */
	 DelTxNode(pf);				/* Delete the MOVAW.	*/
	 ndisc += 1;				/* Increment number discarded.*/
	}
 else						/* Live.	*/
	{lexchin(pf,pl);			/* Adjust  line numbers. */
	 PutTxOperandAd(pl,SOURCE2,adf2);	/* Fix PUSHW source operand. */
	 PutTxOpCodeX(pl,G_PUSHA);		/* Convert PUSHW to PUSHAW. */
	 (void) MoveTxNodeBefore(pl,pf);	/* Flip the instructions. */
	}
 return(TRUE);					/* We did it.	*/
END209:
/************************************************************************/
/*									*/
/* 210 - register replacement:  backward register replacement		*/
/*									*/
/*	op1	A   B   C   R1	->	op1   A   B   C   R2		*/
/*	op2	D   E	R1  R2	->	op2   D   E   R2  R2		*/
/*									*/
/*				or					*/
/*									*/
/*	op1	A   B   C   R1	->	op1   A   B   C   R2		*/
/*	op2	D   R1	E   R2	->	op2   D   R2  E   R2		*/
/*									*/
/*		R1 is dead after op2					*/
/*		R2 is dead after op1, and				*/
/*		where D and E do not use R1				*/
/*									*/
/************************************************************************/

 if(!IsAdCPUReg(adf3))			/* Is first opcode a CPU opcode and */
					/* is first destination a register? */
	goto END210;
 if(!IsAdCPUReg(adl3))			/* Is second opcode a CPU opcode and */
					/* is second destination a register? */
	goto END210;

#ifdef W32200
 if(AutoDepend)					/* Auto Dependencies	*/
	goto END210;				/* too much trouble.	*/
#endif

 if(!IsDeadAd(adf3, pl))		/* Is R1 dead after second instr? */
	goto END210;
 if((adl1 != adf3) && IsAdUses(adl1,adf3)) /* E is R1 or does not use R1. */
					/* Types do not need to match. */
	goto END210;
 if(!IsDeadAd(adl3, pf))		/* Is R2 is dead after first instr? */
	goto END210;
 if((adl2 != adf3) && IsAdUses(adl2,adf3)) /* F is R1 or does not use R1. */
					/* Types do not need to match. */
	goto END210;
 if(adf3 == adl3)			/* R1 != R2, anti-looping constraint */
	goto END210;
 if(!IsOpGeneric(opl))			/* Is second opcode generic? */
	goto END210;
 if(!IsOpGeneric(opf))			/* Is first opcode generic? */
	goto END210;
 if((adl0 != adf3) && IsAdUses(adl0,adf3)) /* D is R1 or does not use R1. */
					/* Types do not need to match. */
	goto END210;
 if(IsOpDstSrc(opf))			/* Does first instr have pure dest? */
	goto END210;
 if(IsOpDstSrc(opl))			/* Does second instr have pure dest? */
	goto END210;
 if(IsOpUses((unsigned short int)opl,adf3)) /* Is dst of op1 used by op2? */
	goto END210;
 if(Pskip(PEEP210))
	goto END210;
	
 peepchange( "propagate registers backward" ,peeppass,210);
 PutTxOperandAd(pf,DESTINATION,adl3);
 for( operand = 0; operand != DESTINATION; operand++ )
	if(GetTxOperandAd(pl,operand) == adf3)
		PutTxOperandAd(pl,operand,adl3);
 return(TRUE);
END210:

/* end of pass 1 optimizations */
if(peeppass == 1)
	return(FALSE);
/**************************************************************************/
/*									  */
/* 220 - addr arith: src immed const and src not reg, disp, immed, or abs.*/
/*									  */
/*	G_ADD/G_SUB/G_MOV   &0  &n  A  B	->  OP        ...C'...    */
/*	OP                  ...C...					  */
/*									  */
/*					or				  */
/*									  */
/*	G_ADD/G_SUB/G_MOV   &0  &n  A  B	->  OP        ...C'...    */
/*	OP	              ...C...      G_ADD/G_SUB/G_MOV &0  &n  A  B */
/*									  */
/*		where							  */
/*			A not a regiser, disp, immed, or abs operand.	  */
/*			C scans all the operands of OP and		  */
/*			C does not use B or uses it in a way that 	  */
/*			can be transformed to accommodate 		  */
/*			moving or deleting the MOV/ADD/SUB.		  */
/*			C is not auto address on A or B.		  */
/*									  */
/*	The object of this optimization is to propagate address		  */
/*	arithmetic through the code and to merge it into other 		  */
/*	instructions as changes to the addressing of the other 		  */
/*	instructions.							  */
/*									  */
/*	The cases for conditions on C are:				  */
/*									  */
/*	I	A	B	C			C'		  */
/*	---	---	---	--------------------	----------------  */
/*									  */
/*	&n	A	B	C == &0			C		  */
/*	&n	A	B	C notaffected by B	C		  */
/*									  */
/*	&n	A	B	C type none		C		  */
/*									  */
/*	&0	A	B	C (pure src) ==	B 	A  (Note 1)	  */
/*									  */
/*	&n	A	B	C (pure dst) == B	C		  */
/*									  */
/*	Combinations for which no mapping is shown are not allowed.       */
/*									  */
/*	Note 1: B is not source of MOVA or PUSHA.			  */
/*									  */
/**************************************************************************/

 if(opf != G_ADD3 && opf != G_SUB3 && opf != G_MOV) /* OP1 is ADD/SUB/MOV */
	goto END220;
 if(IsAdCPUReg(adf2))			/* A not CPU register */
	goto END220;
 if(IsAdDisp(adf2))			/* A not Displacement */
	goto END220;
 if(IsAdImmediate(adf2))		/* A not Immediate */
	goto END220;
 if(IsAdAbsolute(adf2))			/* A not Absolute */
	goto END220;
 if(!IsAdImmediate(adf1) || !IsAdNumber(adf1)) /* &n is immediate constant */
	goto END220;

#ifdef W32200
 if(IsAdAutoAddress(adf3))
	goto END220;
#endif

 if(tyf1 != Tword && tyf1 != Tuword) 	/* &n type */
	goto END220;
 if(tyf2 != Tword && tyf2 != Tuword)	/* A type */
	goto END220;
 if(tyf3 != Tword && tyf3 != Tuword)	/* B type */
	goto END220;
 if(!IsInstrDeleteOrMoveAllowed(pf,pl)) /* Can either delete or move. */
	goto END220;
 if(!(f = IsInstrDeleteAllowed(pf,pl)) 	/* Can either delete or move.*/
		&& !IsInstrMoveAllowed(pf,pl,220))
	goto END220;
 if(Pskip(PEEP220))
	goto END220;

	{int actions[MAX_OPS];		/* per operand actions */
	 AN_Id ad;			/* operand C address id */
	 AN_Id tempad[MAX_OPS];		/* per operand address id's */
	 OperandType ty;		/* operand C type */

	 /* transformation conditions for operands */
	 for(operand = 0; operand < Max_Ops; operand++)
		{ad = GetTxOperandAd(pl,operand); /* address of C */
		 if(ad == i0)			/* C is &0 */
			actions[operand] = AA_NOP; /* C unchanged */
		 else if(notaffected(ad,adf3))	/* C not affected by B */
			{
#ifdef W32200
			 if(IsAdAutoAddress(ad)	/* If auto address */
						/* and uses register */
						/* used by A,*/
			 		&& IsAdUses(adf2,GetAdUsedId(ad,0)))
				goto END220;	/* don't optimize. */
#endif
			 if(operand == DESTINATION 
				&& !f
				&& (IsAdUses(adf2,ad) || IsAdUses(adf3,ad))
			   )
				goto END220;
			 actions[operand] = AA_NOP; /* C unchanged */
			}
		 else if((ty = GetTxOperandType(pl,operand)) == Tnone) /* no C*/
			actions[operand] = AA_NOP; /* C unchanged */
		 else if(operand != DESTINATION
			 && adf1 == i0 		/* n == 0 */
			 && ad == adf3
			 && ty == tyf3
			 && ty != Tuword	/* C (pure src) == B */
			 && opl != G_MOVA	/* B not src of MOVA */
			 && opl != G_PUSHA	/* B not src of PUSHA */
			)
			actions[operand] = AA_COPY; /* C -> A */
		 else if(operand == DESTINATION
			 && ad == adf3 
			 && (TySize(ty) >= szf3 || IsAdCPUReg(ad))
			 && !IsOpDstSrc(opl) 	/* C (pure dst) == B */
			)
			actions[operand] = AA_NOP; /* C -> C */
		 else goto END220;
		}

	 for( operand = 0; operand < Max_Ops; operand++ ) /* transform C */
		 {ad = GetTxOperandAd(pl,operand);	/* address of C */

		  switch(actions[operand])
	 	  	{case AA_NOP:		/* C -> C */
				tempad[operand] = ad;
				endcase;
		  	 case AA_COPY:		/* C -> A */
				tempad[operand] = adf2;
				endcase;
			 default:
				fatal("w2opt(#220): unknown action\n");
		 	}
		}
	 if(IsOpGeneric(opl) && !legalgen((unsigned short) opl, 
		 	tyl0, GetAdMode(tempad[0]),
		 	tyl1, GetAdMode(tempad[1]),
		 	tyl2, GetAdMode(tempad[2]),
		 	tyl3, GetAdMode(tempad[3]))) 
		goto END220;

						/* actually make the change */
	 peepchange( "merge address arith into address modes" ,peeppass,220);
	 for( operand = 1; operand < Max_Ops; operand++ ) 
		PutTxOperandAd(pl,operand,tempad[operand]);
	 if( f ) 			/* delete OP1 */
		{ldelin(pf);
		 DelTxNode(pf);
		 ndisc += 1;			/* Update number discarded. */
	 	}
	 else				/* move OP1 past OP2 */
		{lexchin(pf,pl);
		 (void) MoveTxNodeAfter(pf,pl);	/* exchange */
	 	}
	 return(TRUE);
	}
 END220: 	/* end address arithmetic: indirect references */
/**************************************************************************/
/*									  */
/* 221 - address arithmetic: immediate const source and register source	  */
/*									  */
/*	G_ADD/G_SUB/G_MOV   &0  &n  RA  B ->  OP              ...C'...    */
/*	OP                  ...C...					  */
/*									  */
/*					or				  */
/*									  */
/*	G_ADD/G_SUB/G_MOV   &0  &n  RA  B ->  OP              ...C'...    */
/*	OP	              ...C...     G_ADD/G_SUB/G_MOV &0  &n  RA  B */
/*									  */
/*		where							  */
/*			C scans all the operands of OP and		  */
/*			C does not use B or use it in a way that 	  */
/*			can be transformed to accommodate 		  */
/*			moving or deleting the MOV/ADD/SUB.		  */
/*			C is not auto address on B.			  */
/*									  */
/* 	This optimization keeps track of the amount by which auto address */
/*	operands of OP change RA and compensates in the offsets applied   */
/*	in the displacement mode and in the value of n in the moved	  */
/*	instruction.							  */
/*									  */
/*	The object of this optimization is to propagate address		  */
/*	arithmetic through the code and to merge it into other 		  */
/*	instructions as changes to the addressing of the other 		  */
/*	instructions.							  */
/*									  */
/*	The cases for conditions on C are:				  */
/*									  */
/*	I	A	B	C			C'		  */
/*	---	---	---	--------------------	----------------  */
/*									  */
/*	&n	RA	B	C == &0			C		  */
/*	&n	RA	B	C notaffected by B	C		  */
/*									  */
/*	&0	A	B	C type none		C		  */
/*									  */
/*	&0	RA	B	C (pure src) ==	B 	A  (Note 1)	  */
/*									  */
/*	&n	RA	B	C (pure dst) == B	C		  */
/*									  */
/*	&n	RA	RB	C == expr(RB) 		expr+n(RA) 	  */
/*									  */
/*	&n	RA	RB	C == n(RB) or auto	n(RA) or auto	  */
/*	&n	RA	B	C == *B			n(RA) or auto	  */
/*									  */
/*	&n	RA	B	C (PUSH src) == B	n(RA) (PUSHA src) */
/*									  */
/* 	32200 only:							  */
/*									  */
/*	&n	RA	RB	C == expr(RC1st,RB)	expr+n(RC1st,RA)  */
/*									  */
/*	&n	RA	RB	C == expr(RB,RC2nd)	expr+n(RA,RC2nd)  */
/*									  */
/*	&0	RA	RB	C == RC1st[RB]		RC1st[RA]	  */
/*									  */
/*	&0	RA	RB	C == RB[RC2nd]		RA[RC2nd]	  */
/*									  */
/*	Combinations for which no mapping is shown are not allowed.       */
/*									  */
/*	Note 1: B is not source of MOVA or PUSHA.			  */
/*									  */
/**************************************************************************/

 if(opf != G_ADD3 && opf != G_SUB3 && opf != G_MOV) /* OP1 is ADD/SUB/MOV */
	goto END221;
 if(!IsAdCPUReg(adf2))			/* A is a register. */
	goto END221;
 if(!IsAdImmediate(adf1) || !IsAdNumber(adf1))	/* &n is immediate constant */
	goto END221;
 
#ifdef W32200
 if(IsAdAutoAddress(adf3))		/* B is not auto address. */
	goto END221;
#endif

 if(tyf1 != Tword && tyf1 != Tuword) 	/* &n type */
	goto END221;
 if(tyf2 != Tword && tyf2 != Tuword)	/* A type */
	goto END221;
 if(tyf3 != Tword && tyf3 != Tuword)	/* B type */
	goto END221;
 if(!IsInstrDeleteOrMoveAllowed(pf,pl)) /* Can either delete or move. */
	goto END221;
 if(!(f = IsInstrDeleteAllowed(pf,pl)) 	/* Can either delete or move.*/
		&& !IsInstrMoveAllowed(pf,pl,221))
	goto END221;
 if(Pskip(PEEP221))
	goto END221;

	{int actions[MAX_OPS];		/* per operand actions */
	 AN_Id ad;			/* operand C address id */
	 long int disp_effective;	/* Effective displacement. */
	 long int num_disp[MAX_OPS];	/* Numeric displacement. */
	 char *expr;			/* operand C expression part */
	 long int n_after;		/* immediate operand value after C */
	 long int n_before[MAX_OPS];	/* immediate operand value before C */
	 AN_Id tempad[MAX_OPS];		/* per operand address id's */
	 unsigned int tempop;		/* new opcode index */
	 OperandType ty;		/* operand C type */

#ifdef W32200
	 int sz;			/* operand C size in bytes */
#endif

	 n_after = GetAdNumber(tyf1,adf1);	/* Get immediate value. */
	 if(opf == G_SUB3)			/* Correct for immediate */
		n_after = -n_after; 		/* being subtracted. */
	 					/* Apply transformation */
						/* conditions for operands. */
	 for(operand = 0; operand < Max_Ops; operand++)
		{ty = GetTxOperandType(pl,operand); /* type of C */
		 ad = GetTxOperandAd(pl,operand); /* address of C */
		 n_before[operand] = n_after;	/* n before this operand is */
						/* the value after previous */
						/* operand. */
		 if(ad == i0)			/* C is &0 */
			actions[operand] = AA_NOP;	/* C unchanged */
		 else if(notaffected(ad,adf3))	/* C not affected by B */
			{
#ifdef W32200
			 if(IsAdAutoAddress(ad)	/* If auto address */
			 		&& (GetAdUsedId(ad,0)
					== adf2)) /* and uses RA,*/
						/* get amount of delta. */
				n_after = n_before[operand] 
						- AddressDelta(ty,ad);
#endif
			 if(operand == DESTINATION 
				&& !f
				&& IsAdUses(adf3,ad)
			   )
				goto END221;
			 actions[operand] = AA_NOP;	/* C unchanged */
			}
		 else if((ty = GetTxOperandType(pl,operand)) == Tnone) /* noC */
			actions[operand] = AA_NOP;	/* C unchanged */
		 else if(operand != DESTINATION
			 && adf1 == i0 		/* n == 0 */
			 && ad == adf3
			 && ty == tyf3
			 && ty != Tuword	/* C (pure src) == B */
			 && opl != G_MOVA	/* B not src of MOVA */
			 && opl != G_PUSHA	/* B not src of PUSHA */
			)
			actions[operand] = AA_COPY; 	/* C -> A */
		 else if(operand == DESTINATION
			 && ad == adf3 
			 && (TySize(ty) >= szf3 || IsAdCPUReg(ad))
			 && !IsOpDstSrc(opl) 	/* C (pure dst) == B */
			)
			actions[operand] = AA_NOP;	/* C -> C */
		 else if(GetAdUsedId(ad,0) == adf3 /* C == expr(RB) */
			 && IsAdDisp(ad)
			 && !IsAdNumber(ad)
			)
			actions[operand] = AA_DISP;   /* C -> expr+n(RA) */
		 else if(GetAdUsedId(ad,0) == adf3 	/* C == n(RB) or *B */
			 && GetAdUsedId(ad,1) == NULL
			)
			{			/* Compute the effective */
						/* displacement. */
			 disp_effective = n_before[operand];
			 if(IsAdDisp(ad))
				disp_effective += EffectiveDisp(ty,ad);
						/* Compute everything */
						/* assuming mode stays or */
						/* becomes displacement. */
			 num_disp[operand] = disp_effective;
			 n_after = n_before[operand] + AddressDelta(ty,ad);
			 actions[operand] = AA_NUM_DISP;
						/* See if can use auto */
						/* address to get n closer to */
						/* 0. We only use auto */
						/* addressing if instruction */
						/* is to be moved. */
#ifdef W32200
			 if((cpu_chip == we32200) /* Only for 32200. */
					&& adf2 == adf3 /* and incr/decr */
					&& !f) /* instruction being moved. */
				{sz = TySize(ty);
				 if(disp_effective == sz && n_after >= sz)
					{n_after -= sz;
					 actions[operand] = AA_PRE_INCR;
					}
				 else if((disp_effective == -sz)
						&& (n_after <= -sz))
					{n_after += sz;
					 actions[operand] = AA_PRE_DECR;
					}
				 else if((disp_effective == 0)
						&& (n_after >= sz))
					{n_after -= sz;
					 actions[operand] = AA_POST_INCR;
					}
				 else if((disp_effective == 0)
						&& (n_after <= -sz))
					{n_after += sz;
					 actions[operand] = AA_POST_DECR;
					}
				} /* END OF if((cpu_chip == we32200) && !f) */
#endif
			}
		 else if(adf1 != i0		/* n != 0 */
			 && ad == adf3
			 && opl == G_PUSH	/* C (src of PUSH) == B */
			 && (!f || IsAdSafe(ad) /* not volatile if flipping */
				&& !IsTxOperandVol(pl,operand))
			)
			actions[operand] = AA_PUSHA; /* PUSH C -> PUSHA n(RA) */
#ifdef W32200
		 else if(cpu_chip != we32200)	/* Stop here for 32100. */
			goto END221;
		 else if(GetAdUsedId(ad,1) == adf3 /* B = RC2nd */
			 && IsAdIndexRegDisp(ad)   /* C is index w disp */
			 && IsLegalAd(IndexRegDisp,
				GetAdRegA(ad),
				GetAdRegA(adf2),
				GetAdNumber(ty,ad) + n_before[operand],
				"")		/* Valid Address. */
			)
			actions[operand] = AA_INDEX_REG; 
						/* C -> expr+n(RC1st,RA) */
		 else if(GetAdUsedId(ad,0) == adf3 /* B = RC1st */
			 && IsAdIndexRegDisp(ad)   /* C is index w disp */
			 && IsLegalAd(IndexRegDisp,
				GetAdRegA(adf2),
				GetAdRegB(ad),
				GetAdNumber(ty,ad) + n_before[operand],
				"")		/* Valid Address. */
			)
			actions[operand] = AA_INDEX_REG;
						/* C -> expr+n(RA,RC2nd) */
		 else if(GetAdUsedId(ad,1) == adf3 /* B = RC2nd */
			 && IsAdIndexRegScaling(ad)   /* C is index w scaling */
			 && IsLegalAd(IndexRegScaling,
				GetAdRegA(ad),
				GetAdRegA(adf2),
				GetAdNumber(ty,ad) + n_before[operand],
				"")		/* Valid Address. */
			)
			actions[operand] = AA_INDEX_REG; /* C -> RC1st[RA] */
		 else if(GetAdUsedId(ad,0) == adf3 /* B = RC1st */
			 && IsAdIndexRegScaling(ad)   /* C is index w scaling */
			 && IsLegalAd(IndexRegScaling,
				GetAdRegA(adf2),
				GetAdRegB(ad),
				GetAdNumber(ty,ad) + n_before[operand],
				"")		/* Valid Address. */
			)
			actions[operand] = AA_INDEX_REG; /* C -> RA[RC2nd] */
#endif
		 else goto END221;
		}

	 tempop = opl;				/* trial opcode index */

	 for( operand = 0; operand < Max_Ops; operand++ ) /* transform C */
		 {ty = GetTxOperandType(pl,operand);	/* type of C */
		  ad = GetTxOperandAd(pl,operand);	/* address of C */

		  switch(actions[operand])
	 	  	{case AA_NOP:		/* C -> C */
				tempad[operand] = ad;
				endcase;
		  	 case AA_COPY:		/* C -> A */
				tempad[operand] = adf2;
				endcase;
		  	 case AA_DISP:		/* C -> expr+n(RA) */
				expr = GetAdExpression(ty,ad);
				tempad[operand] = GetAdDispInc(ty,expr,
						GetAdRegA(adf2),
						n_before[operand]);
				endcase;
		  	 case AA_NUM_DISP:	/* C -> n(RA) */
				tempad[operand] = GetAdDispInc(ty,"",
						GetAdRegA(adf2),
						num_disp[operand]);
				endcase;
#ifdef W32200
			 case AA_PRE_INCR:	/* C -> +(RA) */
				tempad[operand] = GetAdPreIncr(ty,"",
					GetAdRegA(adf2));
				endcase;
			 case AA_PRE_DECR:	/* C -> -(RA) */
				tempad[operand] = GetAdPreDecr(ty,"",
					GetAdRegA(adf2));
				endcase;
			 case AA_POST_INCR:	/* C -> (RA)+ */
				tempad[operand] =GetAdPostIncr(ty,"",
					GetAdRegA(adf2));
				endcase;
			 case AA_POST_DECR:	/* C -> (RA)- */
				tempad[operand] =GetAdPostDecr(ty,"",
					GetAdRegA(adf2));
				endcase;
			 case AA_INDEX_REG:	/* C -> expr+n(RC1st,RA) */
						/* C -> expr+n(RA,RC2nd) */
			 			/* C -> RC1st[RA] */
						/* C -> RA[RC2nd] */
				if(GetAdUsedId(ad,1) == adf3)
					 tempad[operand] = GetAdChgRegBInc(ty,
						ad,GetAdRegA(adf2),
						n_before[operand]);
				else
					 tempad[operand] = GetAdChgRegAInc(ty,
						ad,GetAdRegA(adf2),
						n_before[operand]);
				endcase;
#endif
		  	 case AA_PUSHA:		/* PUSH C -> PUSHA n(RA) */
				tempop = G_PUSHA;
				tempad[operand] = GetAdAddIndInc(tyf2,ty,adf2,
						n_before[operand]);
				endcase;
			 default:
				fatal("w2opt(#221): unknown action\n");
		 	}
		}
	 if(IsOpGeneric(tempop) && !legalgen((unsigned short) tempop, 
		 	tyl0, GetAdMode(tempad[0]),
		 	tyl1, GetAdMode(tempad[1]),
		 	tyl2, GetAdMode(tempad[2]),
		 	tyl3, GetAdMode(tempad[3]))) 
		goto END221;

						/* actually make the change */
	 peepchange( "merge address arith into address modes" ,peeppass,221);
	 PutTxOpCodeX(pl,tempop);
	 for( operand = 1; operand < Max_Ops; operand++ ) 
		PutTxOperandAd(pl,operand,tempad[operand]);
	 if( f 				/* Meets general delete conditions. */
		|| (n_after == 0 && adf1 == adf2)  /* Its useless move. */
	   ) 				/* delete OP1 */
		{ldelin(pf);
		 DelTxNode(pf);
		 ndisc += 1;			/* Update number discarded. */
	 	}
	 else				/* Modify OP1 to reflect any change */
					/* in M and move OP1 past OP2. */
		{if(n_after > 0)
			{PutTxOpCodeX(pf,G_ADD3);
			 PutTxOperandAd(pf,1,GetAdAddToKey(tyf1,i0,n_after));
			}
		 else if(n_after == 0)
			{PutTxOpCodeX(pf,G_MOV);
			 PutTxOperandAd(pf,1,i0);
			}
		 else if(n_after < 0)
			{PutTxOpCodeX(pf,G_SUB3);
			 PutTxOperandAd(pf,1,GetAdAddToKey(tyf1,i0,-n_after));
			}
		 lexchin(pf,pl);
		 (void) MoveTxNodeAfter(pf,pl);	/* exchange */
	 	}
	 return(TRUE);
	}
 END221: 	/* end address arithmetic: indirect references */
/************************************************************************/
/*									  */
/* 222 - address arithmetic: immediate const and non-register source	  */
/*									  */
/*	G_ADD/G_SUB/G_MOV   &0  &n  A  B ->  OP               ...C'...    */
/*	OP                  ...C...					  */
/*									  */
/*					or				  */
/*									  */
/*	G_ADD/G_SUB/G_MOV   &0  &n  A  B ->  OP               ...C'...    */
/*	OP	              ...C...     G_ADD/G_SUB/G_MOV &0  &n  RA  B */
/*									  */
/*		where							  */
/*			C scans all the operands of OP and		  */
/*			C does not use B or use it in a way that 	  */
/*			can be transformed to accommodate 		  */
/*			moving or deleting the MOV/ADD/SUB.		  */
/*			C is not auto address on A or B			  */
/*									  */
/*	The object of this optimization is to propagate address		  */
/*	arithmetic through the code and to merge it into other 		  */
/*	instructions as changes to the addressing of the other 		  */
/*	instructions.							  */
/*									  */
/*	The cases for conditions on C are:				  */
/*									  */
/*	I	A	B	C			C'		  */
/*	---	---	---	--------------------	----------------  */
/*									  */
/*	&n	A	B	C == &0			C		  */
/*	&n	A	B	C notaffected by B	C		  */
/*									  */
/*	&0	A	B	C (pure src) ==	B 	A  (Note 1)	  */
/*									  */
/*	&0	A	B	C type none		C		  */
/*									  */
/*	&n	A	B	C (pure dst) == B	C		  */
/*									  */
/*	&0    expr(RA)	RB	C == 0(RB)		*expr(RA)	  */
/*	&0    expr(RA)	B	C == *B			*expr(RA)	  */
/*	&0    &expr	RB	C == 0(RB)		$expr		  */
/*	&0    &expr	B	C == *B			$expr		  */
/*	&0    $expr	RB	C == 0(RB)		*$expr		  */
/*	&0    $expr	B	C == *B			*$expr		  */
/*									  */
/*	Combinations for which no mapping is shown are not allowed.       */
/*									  */
/*	Note 1: B is not source of MOVA or PUSHA.			  */
/*									  */
/**************************************************************************/

 if(opf != G_ADD3 && opf != G_SUB3 && opf != G_MOV) /* OP1 is ADD/SUB/MOV */
	goto END222;
					/* A is expr(RA), &expr, or $expr */
 if(!IsAdDisp(adf2) && !IsAdImmediate(adf2) && !IsAdAbsolute(adf2))
	goto END222;
 if(!IsAdImmediate(adf1) || !IsAdNumber(adf1)) /* &n is immediate constant */
	goto END222;

#ifdef W32200
 if(IsAdAutoAddress(adf3))		/* B not auto address */
	goto END222;
#endif

 if(tyf1 != Tword && tyf1 != Tuword)	/* &n type */
	goto END222;
 if(tyf2 != Tword && tyf2 != Tuword)	/* A type */
	goto END222;
 if(tyf3 != Tword && tyf3 != Tuword)	/* B type */
	goto END222;
 if(!IsInstrDeleteOrMoveAllowed(pf,pl)) /* Can either delete or move. */
	goto END222;
 if(!(f = IsInstrDeleteAllowed(pf,pl)) 	/* Can either delete or move.*/
		&& !IsInstrMoveAllowed(pf,pl,222))
	goto END222;
 if(Pskip(PEEP222))
	goto END222;

	{int actions[MAX_OPS];		/* per operand actions */
	 AN_Id ad;			/* operand C address id */
 	 unsigned int i;		/* operand index */
	 AN_Id tempad[MAX_OPS];		/* per operand address id's */
	 OperandType ty;		/* operand C type */

	 /* transformation conditions for operands */
	 for(i = 0; i < Max_Ops; i++)
		{ad = GetTxOperandAd(pl,i);	/* address of C */
		 if(ad == i0)			/* C is &0 */
			actions[i] = AA_NOP;	/* C unchanged */
		 else if(notaffected(ad,adf3))  /* C not affected by B */
			{
#ifdef W32200
			 if(IsAdAutoAddress(ad)	/* If auto address */
						/* and uses register */
						/* used by A,*/
			 		&& IsAdUses(adf2,GetAdUsedId(ad,0)))
				goto END222;	/* don't optimize. */
#endif
			 if(i == DESTINATION 
				&& !f
				&& (IsAdUses(adf2,ad) || IsAdUses(adf3,ad))
			   )
				goto END222;
			 actions[i] = AA_NOP;	/* C unchanged */
			}
		 else if((ty = GetTxOperandType(pl,i)) == Tnone) /* no C */
			actions[i] = AA_NOP;	/* C unchanged */
		 else if(i != DESTINATION
			 && adf1 == i0 		/* n == 0 */
			 && ad == adf3
			 && ty == tyf3
			 && ty != Tuword	/* C (pure src) == B */
			 && opl != G_MOVA	/* B not src of MOVA */
			 && opl != G_PUSHA	/* B not src of PUSHA */
			)
			actions[i] = AA_COPY; 	/* C -> A */
		 else if(i == DESTINATION
			 && ad == adf3 
			 && (TySize(ty) >= szf3 || IsAdCPUReg(ad))
			 && !IsOpDstSrc(opl) 	/* C (pure dst) == B */
			)
			actions[i] = AA_NOP;	/* C -> C */
		 else if((IsAdDisp(adf2) 
			     || IsAdImmediate(adf2) 
			     || IsAdAbsolute(adf2)
			    )			/* A == expr(RA),&expr,$expr */
			 && adf1 == i0
			 && GetAdUsedId(ad,0) == adf3
			 && IsAdRemInd(ty,ad)
#ifdef W32200
			 && !IsAdAutoAddress(ad) /* C == expr(RB),*B */
#endif
			)
			actions[i] = AA_INDIRECT; /* C -> indirect(A) */
		 else goto END222;
		}

	 for( i = 0; i < Max_Ops; i++ ) 	/* transform C */
		 {ty = GetTxOperandType(pl,i);	/* type of C */
		  ad = GetTxOperandAd(pl,i);	/* address of C */

		  switch(actions[i])
	 	  	{case AA_NOP:		/* C -> C */
				tempad[i] = ad;
				endcase;
		  	 case AA_COPY:		/* C -> A */
				tempad[i] = adf2;
				endcase;
		  	 case AA_INDIRECT:	/* C -> indirect(A) */
				tempad[i] = GetAdAddIndInc(tyf2,ty,adf2,0);
				endcase;
			 default:
				fatal("w2opt(#222): unknown action\n");
		 	}
		}
	 if(IsOpGeneric(opl) && !legalgen((unsigned short) opl, 
		 	tyl0, GetAdMode(tempad[0]),
		 	tyl1, GetAdMode(tempad[1]),
		 	tyl2, GetAdMode(tempad[2]),
		 	tyl3, GetAdMode(tempad[3]))) 
		goto END222;

						/* actually make the change */
	 peepchange( "merge address arith into address modes" ,peeppass,222);
	 for( i = 1; i < Max_Ops; i++ ) 
		PutTxOperandAd(pl,i,tempad[i]);
	 if( f ) 			/* delete OP1 */
		{ldelin(pf);
		 DelTxNode(pf);
		 ndisc += 1;			/* Update number discarded. */
	 	}
	 else				/* move OP1 past OP2 */
		{lexchin(pf,pl);
		 (void) MoveTxNodeAfter(pf,pl);	/* exchange */
	 	}
	 return(TRUE);
	}
 END222: 	/* end address arithmetic: non-register source for move */

#ifdef W32200
/**************************************************************************/
/*									  */
/* 223 - address arithmetic: register sources from different sets	  */
/*									  */
/*	G_ADD   &0  R  RA  B	->  OP     ...C'...		  	  */
/*	OP      ...C...							  */
/*									  */
/*					or				  */
/*									  */
/*	G_ADD   &0  R  RA  B	->  OP     ...C'...			  */
/*	OP      ...C...             G_ADD  &0  R  RA  B			  */
/*									  */
/*		where							  */
/*			C scans all the operands of OP and		  */
/*			C does not use B or use it in a way that 	  */
/*			can be transformed to accommodate 		  */
/*			moving or deleting the ADD.			  */
/*			B is not auto address.				  */
/*			C is not auto address on operand of G_ADD	  */
/*									  */
/*	The object of this optimization is to propagate address		  */
/*	arithmetic through the code and to merge it into other 		  */
/*	instructions as changes to the addressing of the other 		  */
/*	instructions.							  */
/*									  */
/*	The cases for conditions on C are:				  */
/*									  */
/*	R	RA	B	C			C'		  */
/*	---	---	---	--------------------	----------------  */
/*									  */
/*	R	RA	B	C == &0			C		  */
/*	R	RA	B	C notaffected by B	C		  */
/*									  */
/*	R	RA	B	C type none		C		  */
/*									  */
/*	R	RA	B	C (pure dst) == B	C		  */
/*									  */
/*	32200 only:							  */
/*									  */
/*	R	RA	RB	C == n(RB)		n(RA,R)		  */
/*	R	RA	RB	C == n(RB)		n(R,RA)		  */
/*	R	RA	B	C == *B	         	0(RA,R)		  */
/*	R	RA	B	C == *B	         	0(R,RA)		  */
/*									  */
/*	R	RA	B	C (PUSH src) == B    0(RA,R) (PUSHA src)  */
/*	R	RA	B	C (PUSH src) == B    0(R,RA) (PUSHA src)  */
/*									  */
/*	Combinations for which no mapping is shown are not allowed.       */
/*									  */
/**************************************************************************/

 if(cpu_chip != we32200)		/* Two register sets only on 32100 */
	goto END223;
 if(opf != G_ADD3)		 	/* OP1 is ADD */
	goto END223;
 if(!IsAdCPUReg(adf1))			/* R is register */
	goto END223;
 if(!IsAdCPUReg(adf2))			/* RA is register */
	goto END223;
 regnof1 = GetRegNo(GetAdRegA(adf1));	/* Get register number for R. */
 regnof2 = GetRegNo(GetAdRegA(adf2));	/* Get register number for RA. */
 if(regnof1 > 15 && regnof2 > 15)	/* One register from first set. */
	goto END223;
 if(regnof1 < 16 && regnof2 < 16)	/* One register from second set. */
	goto END223;
 if(IsAdAutoAddress(adf3))		/* B not auto address */
	goto END223;
 if(tyf1 != Tword && tyf1 != Tuword)	/* R type */
	goto END223;
 if(tyf2 != Tword && tyf2 != Tuword)	/* RA type */
	goto END223;
 if(tyf3 != Tword && tyf3 != Tuword)	/* B type */
	goto END223;
 if(!IsInstrDeleteOrMoveAllowed(pf,pl)) /* Can either delete or move. */
	goto END223;
 if(!(f = IsInstrDeleteAllowed(pf,pl)) 	/* Can either delete or move.*/
		&& !IsInstrMoveAllowed(pf,pl,223))
	goto END223;
 if(Pskip(PEEP223))
	goto END223;

	{int actions[MAX_OPS];		/* per operand actions */
	 AN_Id ad;			/* operand C address id */
	 char *expr;			/* operand C expression part */
 	 unsigned int i;		/* operand index */
	 AN_Id tempad[MAX_OPS];		/* per operand address id's */
	 unsigned int tempop;		/* new opcode index */
	 OperandType ty;		/* operand C type */

	 /* transformation conditions for operands */
	 for(i = 0; i < Max_Ops; i++)
		{ad = GetTxOperandAd(pl,i);	/* address of C */
		 if(ad == i0)			/* C is &0 */
			actions[i] = AA_NOP;	/* C unchanged */
		 else if(notaffected(ad,adf3))	/* C not affected by B */
			{if(IsAdAutoAddress(ad)	/* If auto address */
			 		&& (GetAdUsedId(ad,0)
					== adf1)) /* and uses R,*/
				goto END223;	/* don't optimize. */
			 if(IsAdAutoAddress(ad)	/* If auto address */
			 		&& (GetAdUsedId(ad,0)
					== adf2)) /* and uses RA,*/
				goto END223;	/* don't optimize. */
			 actions[i] = AA_NOP;	/* C unchanged */
			}
		 else if((ty = GetTxOperandType(pl,i)) == Tnone) /* no C */
			actions[i] = AA_NOP;	/* C unchanged */
		 else if(i == DESTINATION
			 && ad == adf3 
			 && (TySize(ty) >= szf3 || IsAdCPUReg(ad))
			 && !isdstsrcg(opl) 	/* C (pure dst) == B */
			)
			actions[i] = AA_NOP;	/* C -> C */
						/* Begin 32200 optimizations. */
		 else if(!IsFP(ty)
			 && GetAdUsedId(ad,0) == adf3 
			 && GetAdUsedId(ad,1) == NULL 
			 && !IsAdAutoAddress(ad) /* C = n(RB), *B */
			 && IsAdNumber(ad)
			 && IsLegalAd(IndexRegDisp,
				GetAdRegA(adf2),
				GetAdRegA(adf1),
				GetAdNumber(ty,ad),
				"")		/* Valid n(RA,R). */
			)
			actions[i] = AA_INDEX_REG; /* C = n(RA,R) */
		 else if(!IsFP(ty)
			 && GetAdUsedId(ad,0) == adf3 
			 && GetAdUsedId(ad,1) == NULL
			 && !IsAdAutoAddress(ad) /* C = n(RB), *B */
			 && IsAdNumber(ad)
			 && IsLegalAd(IndexRegDisp,
				GetAdRegA(adf1),
				GetAdRegA(adf2),
				GetAdNumber(ty,ad),
				"")		/* Valid n(R,RA). */
			)
			actions[i] = AA_INDEX_REG; /* C = n(R,RA) */
		 else if(!IsFP(ty)
			 && ad == adf3
			 && opl == G_PUSH
			 && IsLegalAd(IndexRegDisp,
				GetAdRegA(adf2),
				GetAdRegA(adf1),
				0,
				"")		/* Valid 0(RA,R). */
			)
			actions[i] = AA_PUSHA;	/* C = 0(RA,R) */
		 else if(!IsFP(ty)
			 && ad == adf3
			 && opl == G_PUSH
			 && IsLegalAd(IndexRegDisp,
				GetAdRegA(adf1),
				GetAdRegA(adf2),
				0,
				"")		/* Valid 0(R,RA). */
			)
			actions[i] = AA_PUSHA;	/* C = 0(RA,R) */
		 else goto END223;
		}

	 tempop = opl;				/* trial opcode index */

	 for( i = 0; i < Max_Ops; i++ ) 	/* transform C */
		 {ty = GetTxOperandType(pl,i);	/* type of C */
		  ad = GetTxOperandAd(pl,i);	/* address of C */

		  switch(actions[i])
	 	  	{case AA_NOP:		/* C -> C */
				tempad[i] = ad;
				endcase;
			 case AA_INDEX_REG:	/* C -> n(RA,R) */
						/* C -> n(R,RA) */
				if(IsAdDisp(ad))
					expr = GetAdExpression(ty,ad);
				else
					expr = GetAdExpression(ty,i0);
				if(regnof1 < 15)
					tempad[i] = GetAdIndexRegDisp(ty,expr,
						GetAdRegA(adf2),
						GetAdRegA(adf1));
				else
					tempad[i] = GetAdIndexRegDisp(ty,expr,
						GetAdRegA(adf1),
						GetAdRegA(adf2));
				endcase;
			 case AA_PUSHA:		/* C -> 0(RA,R) (PUSHA src) */
						/* C -> 0(R,RA) (PUSHA src) */
				tempop = G_PUSHA;
				if(regnof1 < 15)
					tempad[i] = GetAdIndexRegDisp(ty,"",
						GetAdRegA(adf2),
						GetAdRegA(adf1));
				else
					tempad[i] = GetAdIndexRegDisp(ty,"",
						GetAdRegA(adf1),
						GetAdRegA(adf2));
				endcase;
			 default:
				fatal("w2opt(#223): unknown action\n");
		 	}
		}
	 if(IsOpGeneric(tempop) && !legalgen((unsigned short) tempop, 
		 	tyl0, GetAdMode(tempad[0]),
		 	tyl1, GetAdMode(tempad[1]),
		 	tyl2, GetAdMode(tempad[2]),
		 	tyl3, GetAdMode(tempad[3]))) 
		goto END223;

						/* actually make the change */
	 peepchange( "merge address arith into address modes" ,peeppass,223);
	 PutTxOpCodeX(pl,tempop);
	 for( i = 1; i < Max_Ops; i++ ) 
		PutTxOperandAd(pl,i,tempad[i]);
	 if( f ) 			/* delete OP1 */
		{ldelin(pf);
		 DelTxNode(pf);
		 ndisc += 1;			/* Update number discarded. */
	 	}
	 else				/* move OP1 past OP2 */
		{lexchin(pf,pl);
		 (void) MoveTxNodeAfter(pf,pl);	/* exchange */
	 	}
	 return(TRUE);
	}
 END223: 	/* end address arithmetic: non-register source for move */
#endif

#ifdef W32200
/**************************************************************************/
/*									  */
/* 224 - address arithmetic: sources same register from 1st set		  */
/*									  */
/*	G_ADD   &0  R  RA  RB	->  OP     ...t C'...		  	  */
/*	OP      ...t C...						  */
/*									  */
/*					or				  */
/*									  */
/*	G_ADD   &0  R  RA  RB	->  OP     ...t C'...			  */
/*	OP      ...t C...             G_ADD  &0  R  RA  RB		  */
/*									  */
/*		where							  */
/*			C scans all the operands of OP and		  */
/*			C does not use B or use it in a way that 	  */
/*			can be transformed to accommodate 		  */
/*			moving or deleting the ADD.			  */
/*			C is not auto address on A or B			  */
/*			t is half or uhalf				  */
/*									  */
/*	The object of this optimization is to propagate address		  */
/*	arithmetic through the code and to merge it into other 		  */
/*	instructions as changes to the addressing of the other 		  */
/*	instructions.							  */
/*									  */
/*	The cases for conditions on C are:				  */
/*									  */
/*	R       RA	RB	C			C'		  */
/*	---	---	---	--------------------	----------------  */
/*									  */
/*	R	R	RB	C == &0			C		  */
/*	R	R	RB	C notaffected by B	C		  */
/*									  */
/*	R	R	RB	C type none		C		  */
/*									  */
/*	R	R	RB	C (pure dst) == B	C		  */
/*									  */
/*	32200 only:							  */
/*									  */
/*	R	R	RB	C == 0(RC1st,RB)	RC1st[R]	  */
/*									  */
/*	Combinations for which no mapping is shown are not allowed.       */
/*									  */
/**************************************************************************/

 if(cpu_chip != we32200)		/* Indexed modes only on 32200. */
	goto END224;
 if(opf != G_ADD3)		 	/* OP1 is ADD/SUB */
	goto END224;
 if(!IsAdCPUReg(adf1))			/* R is register */
	goto END224;
 if(adf2 != adf1)			/* RA == R */
	goto END224;
 if(!IsAdCPUReg(adf3))			/* RB is register */
	goto END224;
 regnof2 = GetRegNo(GetAdRegA(adf2));	/* Get register number for R. */
 if(regnof2 > 15)			/* R is from 1st set. */
	goto END224;
 if(tyf1 != Tword && tyf1 != Tuword)	/* R type */
	goto END224;
 if(tyf2 != Tword && tyf2 != Tuword)	/* RA type */
	goto END224;
 if(tyf3 != Tword && tyf3 != Tuword)	/* RB type */
	goto END224;
 if(!IsInstrDeleteOrMoveAllowed(pf,pl)) /* Can either delete or move. */
	goto END224;
 if(!(f = IsInstrDeleteAllowed(pf,pl)) 	/* Can either delete or move.*/
		&& !IsInstrMoveAllowed(pf,pl,224))
	goto END224;
 if(Pskip(PEEP224))
	goto END224;

	{int actions[MAX_OPS];		/* per operand actions */
	 AN_Id ad;			/* operand C address id */
 	 unsigned int i;		/* operand index */
	 AN_Id tempad[MAX_OPS];		/* per operand address id's */
	 OperandType ty;		/* operand C type */

	 /* transformation conditions for operands */
	 for(i = 0; i < Max_Ops; i++)
		{ad = GetTxOperandAd(pl,i);	/* address of C */
		 if(ad == i0)			/* C is &0 */
			actions[i] = AA_NOP;	/* C unchanged */
		 else if(notaffected(ad,adf3))	/* C not affected by B */
			{if(IsAdAutoAddress(ad)	/* If auto address */
			 		&& (GetAdUsedId(ad,0)
					== adf2)) /* and uses A,*/
				goto END224;	/* don't optimize. */
			 actions[i] = AA_NOP;	/* C unchanged */
			}
		 else if((ty = GetTxOperandType(pl,i)) == Tnone) /* no C */
			actions[i] = AA_NOP;	/* C unchanged */
		 else if(i == DESTINATION
			 && ad == adf3 
			 && (TySize(ty) >= szf3 || IsAdCPUReg(ad))
			 && !IsOpDstSrc(opl) 	/* C (pure dst) == B */
			)
			actions[i] = AA_NOP;	/* C -> C */
		 else if((ty == Thalf ||  ty == Tuhalf)
			 && IsAdIndexRegDisp(ad)
			 && IsAdNumber(ad)
			 && GetAdNumber(ty,ad) == 0
			 && GetAdRegB(ad) 
				== GetAdRegA(adf3) /* C = 0(RC1st,RB) */
			)
			actions[i] = AA_INDEX_REG; /* C -> RC1st[R] */
		 else goto END224;
		}

	 for( i = 0; i < Max_Ops; i++ ) 	/* transform C */
		 {ty = GetTxOperandType(pl,i);	/* type of C */
		  ad = GetTxOperandAd(pl,i);	/* address of C */

		  switch(actions[i])
	 	  	{case AA_NOP:		/* C -> C */
				tempad[i] = ad;
				endcase;
			 case AA_INDEX_REG:	/* C -> RC1st[R] */
				tempad[i] = GetAdIndexRegScaling(ty,"",
					GetAdRegA(ad),GetAdRegA(adf2));
				endcase;
			 default:
				fatal("w2opt(#224): unknown action\n");
		 	}
		}
	 if(IsOpGeneric(opl) && !legalgen((unsigned short) opl, 
		 	tyl0, GetAdMode(tempad[0]),
		 	tyl1, GetAdMode(tempad[1]),
		 	tyl2, GetAdMode(tempad[2]),
		 	tyl3, GetAdMode(tempad[3]))) 
		goto END224;

						/* actually make the change */
	 peepchange( "merge address arith into address modes" ,peeppass,224);
	 for( i = 1; i < Max_Ops; i++ ) 
		PutTxOperandAd(pl,i,tempad[i]);
	 if( f ) 			/* delete OP1 */
		{ldelin(pf);
		 DelTxNode(pf);
		 ndisc += 1;			/* Update number discarded. */
	 	}
	 else				/* move OP1 past OP2 */
		{lexchin(pf,pl);
		 (void) MoveTxNodeAfter(pf,pl);	/* exchange */
	 	}
	 return(TRUE);
	}
 END224: 	/* end address arithmetic: sources same register from 1st set */
#endif

#ifdef W32200
/**************************************************************************/
/*									  */
/* 225 - address arithmetic: shift by 2					  */
/*									  */
/*	G_ALS/LLS   &0  &2  RA  RB	->  OP     ...t C'...	  	  */
/*	OP          ...t C...						  */
/*									  */
/*					or				  */
/*									  */
/*	G_ALS/LLS   &0  &2  RA  RB	->  OP     ...t C'...		  */
/*	OP      ...t C...             	    G_ALS/LLS  &0  &2  RA  RB	  */
/*									  */
/*		where							  */
/*			C scans all the operands of OP and		  */
/*			C does not use B or use it in a way that 	  */
/*			can be transformed to accommodate 		  */
/*			moving or deleting the ALS/LLS.			  */
/*			C is not auto address on A or B			  */
/*			t is word or uword				  */
/*									  */
/*	The object of this optimization is to propagate address		  */
/*	arithmetic through the code and to merge it into other 		  */
/*	instructions as changes to the addressing of the other 		  */
/*	instructions.							  */
/*									  */
/*	The cases for conditions on C are:				  */
/*									  */
/*	&2      RA	RB	C			C'		  */
/*	---   	---	---	--------------------	----------------  */
/*									  */
/*	&2	RA	RB	C == &0			C		  */
/*	&2 	RA	RB	C notaffected by B	C		  */
/*									  */
/*	&2	RA	RB	C type none		C		  */
/*									  */
/*	&2	RA	RB	C (pure dst) == B	C		  */
/*									  */
/*	32200 only:							  */
/*									  */
/*	&2	RA	RB	C == 0(RC1st,RB)	RC1st[RA]	  */
/*									  */
/*	Combinations for which no mapping is shown are not allowed.       */
/*									  */
/**************************************************************************/

 if(cpu_chip != we32200)		/* Indexed modes only on 32200. */
	goto END225;
 if(opf != G_ALS3 && opf != G_LLS3)	/* OP1 is ALS/LLS */
	goto END225;
 if(!IsAdImmediate(adf1))
	goto END225;
 if(!IsAdNumber(adf1))
	goto END225;
 if(GetAdNumber(tyf1,adf1) != 2)
	goto END225;			/* Immediate 2 */
 if(!IsAdCPUReg(adf2))			/* RA is register */
	goto END225;
 if(!IsAdCPUReg(adf3))			/* RB is register */
	goto END225;
 regnof2 = GetRegNo(GetAdRegA(adf2));	/* Get register number for RA. */
 if(regnof2 > 15)			/* RA is from 1st set. */
	goto END225;
 if(tyf1 != Tword && tyf1 != Tuword)	/* &2 type */
	goto END225;
 if(tyf2 != Tword && tyf2 != Tuword)	/* RA type */
	goto END225;
 if(tyf3 != Tword && tyf3 != Tuword)	/* RB type */
	goto END225;
 if(!IsInstrDeleteOrMoveAllowed(pf,pl)) /* Can either delete or move. */
	goto END225;
 if(!(f = IsInstrDeleteAllowed(pf,pl)) 	/* Can either delete or move.*/
		&& !IsInstrMoveAllowed(pf,pl,225))
	goto END225;
 if(Pskip(PEEP225))
	goto END225;

	{int actions[MAX_OPS];		/* per operand actions */
	 AN_Id ad;			/* operand C address id */
 	 unsigned int i;		/* operand index */
	 AN_Id tempad[MAX_OPS];		/* per operand address id's */
	 OperandType ty;		/* operand C type */

	 /* transformation conditions for operands */
	 for(i = 0; i < Max_Ops; i++)
		{ad = GetTxOperandAd(pl,i);	/* address of C */
		 if(ad == i0)			/* C is &0 */
			actions[i] = AA_NOP;	/* C unchanged */
		 else if(notaffected(ad,adf3))	/* C not affected by B */
			{if(IsAdAutoAddress(ad)	/* If auto address */
			 		&& (GetAdUsedId(ad,0)
					== adf2)) /* and uses A,*/
				goto END225;	/* don't optimize. */
			 actions[i] = AA_NOP;	/* C unchanged */
			}
		 else if((ty = GetTxOperandType(pl,i)) == Tnone) /* no C */
			actions[i] = AA_NOP;	/* C unchanged */
		 else if(i == DESTINATION
			 && ad == adf3 
			 && (TySize(ty) >= szf3 || IsAdCPUReg(ad))
			 && !IsOpDstSrc(opl) 	/* C (pure dst) == B */
			)
			actions[i] = AA_NOP;	/* C -> C */
		 else if((ty == Tword || ty == Tuword)	/* t is word or uword */
			 && IsAdIndexRegDisp(ad)
			 && IsAdNumber(ad)
			 && GetAdNumber(ty,ad) == 0
			 && GetAdRegB(ad) 
				== GetAdRegA(adf3) /* C = 0(RC1st,RB) */
			)
			actions[i] = AA_INDEX_REG; /* C -> RC1st[R] */
		 else goto END225;
		}

	 for( i = 0; i < Max_Ops; i++ ) 	/* transform C */
		 {ty = GetTxOperandType(pl,i);	/* type of C */
		  ad = GetTxOperandAd(pl,i);	/* address of C */

		  switch(actions[i])
	 	  	{case AA_NOP:		/* C -> C */
				tempad[i] = ad;
				endcase;
			 case AA_INDEX_REG:	/* C -> RC1st[R] */
				tempad[i] = GetAdIndexRegScaling(ty,"",
					GetAdRegA(ad),GetAdRegA(adf2));
				endcase;
			 default:
				fatal("w2opt(#225): unknown action\n");
		 	}
		}
	 if(IsOpGeneric(opl) && !legalgen((unsigned short) opl, 
		 	tyl0, GetAdMode(tempad[0]),
		 	tyl1, GetAdMode(tempad[1]),
		 	tyl2, GetAdMode(tempad[2]),
		 	tyl3, GetAdMode(tempad[3]))) 
		goto END225;

						/* actually make the change */
	 peepchange( "merge address arith into address modes" ,peeppass,225);
	 for( i = 1; i < Max_Ops; i++ ) 
		PutTxOperandAd(pl,i,tempad[i]);
	 if( f ) 			/* delete OP1 */
		{ldelin(pf);
		 DelTxNode(pf);
		 ndisc += 1;			/* Update number discarded. */
	 	}
	 else				/* move OP1 past OP2 */
		{lexchin(pf,pl);
		 (void) MoveTxNodeAfter(pf,pl);	/* exchange */
	 	}
	 return(TRUE);
	}
 END225: 	/* end address arithmetic: sources same register from 1st set */
#endif
/**************************************************************************/
/*									  */
/* 227 - merge type conversions						  */
/*									  */
/*	G_move   	&0  &n  A  B	->  OP            ...C'...	  */
/*	OP                  ...C...					  */
/*									  */
/*					or				  */
/*									  */
/*	G_move   	&0  &n  A  B	->  OP            ...C'...	  */
/*	OP	              ...C...       G_move 	&0  &n  A  B 	  */
/*									  */
/*		where							  */
/*			size of A < size of B				  */
/*			C scans all the operands of OP and		  */
/*			C does not use B or uses it in a way that 	  */
/*			can be transformed to accommodate 		  */
/*			moving or deleting the G_move.			  */
/*			C is not auto address on A or B.		  */
/*									  */
/*	The object of this optimization is to type conversions		  */
/*	through the code and to merge them into other 			  */
/*	instructions as changes to the operand types of the other	  */
/*	instructions.							  */
/*									  */
/*	The cases for conditions on C are:				  */
/*									  */
/*	I	A	B	C			C'		  */
/*	---	---	---	--------------------	----------------  */
/*									  */
/*	&n	A	B	C == &0			C		  */
/*	&n	A	B	C notaffected by B	C		  */
/*									  */
/*	&n	A	B	C type none		C		  */
/*									  */
/*	&0	A	B	C (pure src) ==	B 	A  (Note 1)	  */
/*									  */
/*	&n	A	B	C (pure dst) == B	C		  */
/*									  */
/*	Combinations for which no mapping is shown are not allowed.       */
/*									  */
/*	Note 1: B is not source of MOVA or PUSHA.			  */
/*									  */
/**************************************************************************/

 if(!IsOpGMove(opf)) 			/* OP1 is generic move */
	goto END227;

#ifdef W32200
 if(IsAdAutoAddress(adf3))		/* B not auto address */
	goto END227;
#endif

 if(szf2 >= szf3)			/* size(A) < size(B) */
	goto END227;
 if(!IsInstrDeleteOrMoveAllowed(pf,pl)) /* Can either delete or move. */
	goto END227;
 if(!(f = IsInstrDeleteAllowed(pf,pl)) 	/* Can either delete or move.*/
		&& !IsInstrMoveAllowed(pf,pl,227))
	goto END227;
 if(Pskip(PEEP227))
	goto END227;

	{AN_Id ad;			/* operand C address id */
	 AN_Id newad;			/* trial operand address */
	 OperandType newty;		/* trial operand type */
	 AN_Id tempad[MAX_OPS];		/* per operand address id's */
	 OperandType tempty[MAX_OPS];	/* per operand types */
	 OperandType ty;		/* operand C type */

	 /* transformation conditions for operands */
	 for(operand = 0; operand < Max_Ops; operand++)
		{ad = GetTxOperandAd(pl,operand); /* address of C */
		 ty = GetTxOperandType(pl,operand); /* type of C */
		 if(ad == i0)			/* C is &0 */
			{tempty[operand] = ty;	/* C unchanged. */
			 tempad[operand] = ad;
			}
		 else if(notaffected(ad,adf3))	/* C not affected by B */
			{
#ifdef W32200
			 if(IsAdAutoAddress(ad)	/* If auto address */
						/* and uses register */
						/* used by A,*/
			 		&& IsAdUses(adf2,GetAdUsedId(ad,0)))
				goto END227;	/* don't optimize. */
#endif
			 if(operand == DESTINATION
				&& !f
				&& (IsAdUses(adf2,ad) || IsAdUses(adf3,ad))
			   )
				goto END227;
			 tempty[operand] = ty;	/* Otherwise C unchanged. */
			 tempad[operand] = ad;
			}
		 else if((ty = GetTxOperandType(pl,operand)) == Tnone) /* no C*/
			{tempty[operand] = ty;	/* C unchanged. */
			 tempad[operand] = ad;
			}
		 else if(operand != DESTINATION
			 	&& ad == adf3	/* C (pure src) == B */
						/* Check B and C types. */
			 	&& chktyp(pf,2,pl,operand,&newty,&newad)
			 	&& opl != G_MOVA /* B not src of MOVA */
			 	&& opl != G_PUSHA) /* B not src of PUSHA */
			{tempty[operand] = newty; /* C -> A */
			 tempad[operand] = newad;
			}
		 else if(operand == DESTINATION
			 	&& ad == adf3 
			 	&& (TySize(ty) >= szf3 || IsAdCPUReg(ad))
			 	&& !IsOpDstSrc(opl)) /* C (pure dst) == B */
			{tempty[operand] = ty; /* C unchanged. */
			 tempad[operand] = ad;
			}
		 else goto END227;
		}
					/* Check if instruction is legal. */
	 if(IsOpGeneric(opl) && !legalgen((unsigned short) opl, 
		 	tempty[0],GetAdMode(tempad[0]),
		 	tempty[1],GetAdMode(tempad[1]),
		 	tempty[2],GetAdMode(tempad[2]),
		 	tempty[3],GetAdMode(tempad[3]))) 
		goto END227;

						/* actually make the change */
	 peepchange( "merge address arith into address modes" ,peeppass,227);
	 for( operand = 1; operand < Max_Ops; operand++ ) 
		{PutTxOperandType(pl,operand,tempty[operand]);
		 PutTxOperandAd(pl,operand,tempad[operand]);
		}
	 if( f ) 			/* delete OP1 */
		{ldelin(pf);
		 DelTxNode(pf);
		 ndisc += 1;			/* Update number discarded. */
	 	}
	 else				/* move OP1 past OP2 */
		{lexchin(pf,pl);
		 (void) MoveTxNodeAfter(pf,pl);	/* exchange */
	 	}
	 return(TRUE);
	}
 END227: 	/* merge type changes */
/* 228 - address arithmetic: combining or reversing increments/decrements
 *
 *	G_ADD3	&0  &n  A   A	->	G_ADD3   &0   &n+m   A   A
 *	G_ADD3	&0  &m	A   A
 *
 *	G_ADD3	&0  &n  A   A	->	G_ADD3   &0  B   A   A
 *	G_ADD3	&0  B	A   A		G_ADD3   &0  &n  A   A
 *
 *	Similarly for G_SUB3 or a combination of the two.
 */

#ifdef W32200
if(AutoAddress) goto END228;	/* TEMPORARY FIX.	*/
#endif

 if((opf != G_ADD3) && (opf != G_SUB3))		/* ADD or SUB */
	goto END228;
 if(adf3 != adl2)
	goto END228;
 if((opl != G_ADD3) && (opl != G_SUB3))
	goto END228;
 if(!IsAdImmediate(adf1))			/* Constant increment. */
	goto END228;
 if(!IsAdNumber(adf1))
	goto END228;
 if(adf2 != adf3)				 /* A==A==A==A */
	goto END228;
 if(adl2 != adl3)
	goto END228;
 if(tyf2 != tyf3) 
	goto END228;
 if(tyf3 != tyl2) 
	goto END228;
 if(tyl2 != tyl3)
	goto END228;
 if(!IsAdSafe(adf2) || IsTxOperandVol(pf,2))	/* Must not be volatile. */
	goto END228;

#ifdef W32200
 if(IsAdAutoAddress(adf2))			/* If "A" is AutoAddress, */
	goto END228;				/* we cannot do this because */
						/* we need to diddle the */
						/* register.	*/
#endif

 f = (boolean) (IsAdImmediate(adl1) && IsAdNumber(adl1)); /* combine */
 if(!f && !notaffected(adl1,adf3))		/* reverse */
	goto END228;
 if(Pskip(PEEP228))
	goto END228;
	{long int nf, nl;

	 peepchange( "combine or reverse increments/decrements" ,peeppass,228);
	 if(f)					/* combine */
		{nf = GetAdNumber(tyf1,adf1);
		 nl = GetAdNumber(tyf1,adl1);
		 if(opf == G_SUB3)
			nf = -nf;
		 if(opl == G_SUB3)
			nl = -nl;
		 PutTxOpCodeX(pl,G_ADD3);
		 PutTxOperandAd(pl,1,GetAdAddToKey(Tword,i0,nf + nl));
		 lmrgin3( pf, pl, pl );
		 DelTxNode(pf);
		 ndisc += 1;			/* Update number discarded. */
		}
	 else						/* reverse */
		{lexchin( pf, pl );
	 	 (void) MoveTxNodeAfter(pf,pl);
		}
	 return(TRUE);
	}
END228:

#ifdef W32200
/************************************************************************/
/*									*/
/* 230 - try to use auto incr/decr addressing modes where possible	*/
/*	when the incrementing/decrementing follows the generic op.	*/
/*									*/
/*		AUTO INCR/DECR OPTIMIZATION STRATEGY			*/
/*									*/
/*	Requirements on new Peephole 230:				*/
/*									*/
/*	G_op	...t 0(R)...		   ->	G_op	...t (R)-...	*/
/*	G_ADD3	&0	&-b	R	R				*/
/*									*/
/*	G_op	...t 0(R)...		   ->	G_op	...t (R)+...	*/
/*	G_ADD3	&0	&b	R	R				*/
/*									*/
/*	G_op	...t -b(R)...		   ->	G_op	...t -(R)...	*/
/*	G_ADD3	&0	&-b	R	R				*/
/*									*/
/*	G_op	...t b(R)...		   ->	G_op	...t +(R)...	*/
/*	G_ADD3	&0	&b	R	R				*/
/*									*/
/*	G_op	...t 0(R)...		   ->	G_op	...t (R)-...	*/
/*	G_SUB3	&0	&b	R	R				*/
/*									*/
/*	G_op	...t 0(R)...		   ->	G_op	...t (R)+...	*/
/*	G_SUB3	&0	&-b	R	R				*/
/*									*/
/*	G_op	...t -b(R)...		   ->	G_op	...t -(R)...	*/
/*	G_SUB3	&0	&b	R	R				*/
/*									*/
/*	G_op	...t b(R)...		   ->	G_op	...t +(R)...	*/
/*	G_SUB3	&0	&-b	R	R				*/
/*									*/
/*		where							*/
/*									*/
/*			b == TySize(t)					*/
/*									*/
/*			t is not a floating-point type			*/
/*									*/
/*			All types in G_ADD3 and G_SUB3 are 		*/
/*				Tword or Tuword. 			*/
/*									*/
/*			...xxx... is last explicit use of R in G_op.	*/
/*									*/
/************************************************************************/

 if(cpu_chip != we32200)			/* If not a WE32200 CPU chip, */
	goto END230;				/* forget it, because we */
						/* promote addresses to	*/
						/* 32200 modes.	*/
 if(!IsOpGeneric(opf))				/* If first operator is not */
	goto END230;				/* generic, forget it.	*/
 if(!IsAdImmediate(adl1))			/* If "first" operand of*/
	goto END230;				/* G_ADD3 or G_SUB3 is not */
						/* immediate, forget it.*/
 if(!IsAdNumber(adl1))				/* If "first operand of */
	goto END230;				/* G_ADD3 or G_SUB3 is not */
						/* a number, forget it.	*/
 if(!IsAdCPUReg(adl2))				/* If "second" operand of*/
	goto END230;				/* G_ADD3 or G_SUB3 is not */
						/* a CPU register, forget it.*/
 if(adl2 != adl3)				/* If "third" operand of*/
	goto END230;				/* G_ADD3 or G_SUB3 is not */
						/* the same as the "second", */
						/* forget it.	*/
						/* Find last use of R in */
						/* second instruction.	*/
 type = Tnone;					/* We don't yet know type. */
 limit = GetOpFirstOp(opf);			/* Operand index of first opd.*/
 for(operand = Max_Ops - 1;			/* Examine all operands. */
		(int) operand >= limit;		/* This is ALLOP,	*/
		operand--)			/* only backwards.	*/
	{an_id = GetTxOperandAd(pf,operand);	/* Get operand address.	*/
	 if(!IsAdUses(an_id,adl3))		/* Does it use the register? */
		continue;			/* No: try next operand. */
	 type = GetTxOperandType(pf,operand);	/* Get operand type.	*/
	 break;
	} /* END OF for(ALLOP[backwards](pf,operand)) */
 if(type == Tnone)				/* If no displacement is */
	goto END230;				/* from R, forget it.	*/
 if(!IsAdDisp(an_id))				/* If not Displacement mode, */
	goto END230;				/* forget it.	*/
 if(IsFP(type))					/* If floating-point operand,*/
	goto END230;				/* forget it.	*/
 bt = TySize(type);
 bi = GetAdNumber(Tbyte,adl1);
 if((bt != bi) && (bt != -bi))
	goto END230;
 if(!IsAdNumber(an_id))				/* If displacement not a pure */
	goto END230;				/* number, forget it.	*/
 if(setslivecc(pl))				/* If condition codes are */
	goto END230;				/* needed, forget it. */
 if(Pskip(PEEP230))				/* Do we want to do this? */
	goto END230;				/* No.	*/
 displacement = GetAdNumber(type,an_id);	/* If it is, get it.	*/
 regid = GetAdRegA(adl3);
 if(opl == G_ADD3)				/* Handle G_ADD3 case.	*/
	{if(bi < 0)
		{if(displacement == 0)		/* Case 1.	*/
			{an_id = GetAdPostDecr(type,"",regid);
			}
		 else if(displacement == bi)	/* Case 3.	*/
			{an_id = GetAdPreDecr(type,"",regid);
			}
		 else
			goto END230;
		} /* END OF if(bi < 0) */
	 else if(bi > 0)
		{if(displacement == 0)		/* Case 2.	*/
			{an_id = GetAdPostIncr(type,"",regid);
			}
		 else if(displacement == bi)	/* Case 4.	*/
			{an_id = GetAdPreIncr(type,"",regid);
			}
		 else
			goto END230;
		} /* END OF else if(bi > 0) */
	 else
		goto END230;
	} /* END OF if(opl == G_ADD3) */
 else if(opl == G_SUB3)				/* Handle G_SUB3 case.	*/
	{if(bi > 0)
		{if(displacement == 0)		/* Case 5.	*/
			{an_id = GetAdPostDecr(type,"",regid);
			}
		 else if(displacement == -bi)	/* Case 7.	*/
			{an_id = GetAdPreDecr(type,"",regid);
			}
		 else
			goto END230;
		} /* END OF if(bi > 0) */
	 else if(bi < 0)
		{if(displacement == 0)		/* Case 6.	*/
			{an_id = GetAdPostIncr(type,"",regid);
			}
		 else if(displacement == -bi)	/* Case 8.	*/
			{an_id = GetAdPreIncr(type,"",regid);
			}
		 else
			goto END230;
		} /* END OF if(bi < 0) */
	 else
		goto END230;
	} /* END OF else if(opl == GSUB3) */
 else
	goto END230;				/* Not G_ADD3 or G_SUB3. */
 peepchange("use auto-address modes following",peeppass,230);
 PutTxOperandAd(pf,operand,an_id);		/* Replace address of operand */
						/* in second instruction. */
 DelTxNode(pl);					/* Delete G_ADD3 or G_SUB3. */
 ndisc += 1;					/* Update number discarded. */
 return(TRUE);					/* (We did it.)	*/
END230:
#endif

/* end of pass 1 and 2 optimizations */
if(peeppass <= 2)
	return(FALSE);

#ifdef W32200
/************************************************************************/
/*									*/
/* 235 - try to use auto incr/decr addressing modes where possible	*/
/*	when the incrementing/decrementing precedes the generic op.	*/
/*									*/
/*		AUTO INCR/DECR OPTIMIZATION STRATEGY			*/
/*									*/
/*	Requirements on new Peephole 235:				*/
/*									*/
/*	G_ADD3	&0	&-b	R	R				*/
/*	G_op	...t 0(R)...		   ->	G_op	...t -(R)...	*/
/*									*/
/*	G_ADD3	&0	&b	R	R				*/
/*	G_op	...t 0(R)...		   ->	G_op	...t +(R)...	*/
/*									*/
/*	G_ADD3	&0	&-b	R	R				*/
/*	G_op	...t b(R)...		   ->	G_op	...t (R)-...	*/
/*									*/
/*	G_ADD3	&0	&b	R	R				*/
/*	G_op	...t -b(R)...		   ->	G_op	...t (R)+...	*/
/*									*/
/*	G_SUB3	&0	&b	R	R				*/
/*	G_op	...t 0(R)...		   ->	G_op	...t -(R)...	*/
/*									*/
/*	G_SUB3	&0	&-b	R	R				*/
/*	G_op	...t 0(R)...		   ->	G_op	...t +(R)...	*/
/*									*/
/*	G_SUB3	&0	&b	R	R				*/
/*	G_op	...t b(R)...		   ->	G_op	...t (R)-...	*/
/*									*/
/*	G_SUB3	&0	&-b	R	R				*/
/*	G_op	...t -b(R)...		   ->	G_op	...t (R)+...	*/
/*									*/
/*		where							*/
/*									*/
/*			b == TySize(t)					*/
/*									*/
/*			t is not a floating-point type			*/
/*									*/
/*			All types in G_ADD3 and G_SUB3 are 		*/
/*				Tword or Tuword. 			*/
/*									*/
/*			...xxx... is first explicit use of R in G_op.	*/
/*									*/
/************************************************************************/

 if(cpu_chip != we32200)			/* If not a WE32200 CPU chip, */
	goto END235;				/* forget it, because we */
						/* promote addresses to	*/
						/* 32200 modes.	*/
 if(!IsOpGeneric(opl))				/* If second operator is not */
	goto END235;				/* generic, forget it.	*/
 if(!IsAdImmediate(adf1))			/* If "first" operand of*/
	goto END235;				/* G_ADD3 or G_SUB3 is not */
						/* immediate, forget it.*/
 if(!IsAdNumber(adf1))				/* If "first operand of */
	goto END235;				/* G_ADD3 or G_SUB3 is not */
						/* a number, forget it.	*/
 if(!IsAdCPUReg(adf2))				/* If "second" operand of*/
	goto END235;				/* G_ADD3 or G_SUB3 is not */
						/* a CPU register, forget it.*/
 if(adf2 != adf3)				/* If "third" operand of*/
	goto END235;				/* G_ADD3 or G_SUB3 is not */
						/* the same as the "second", */
						/* forget it.	*/
						/* Find first use of R in */
						/* second instruction.	*/
 type = Tnone;					/* We don't yet know type. */
 for(ALLOP(pl,operand))				/* Examine all operands. */
	{an_id = GetTxOperandAd(pl,operand);	/* Get operand address.	*/
	 if(!IsAdUses(an_id,adf3))		/* Does it use the register? */
		continue;			/* No: try next operand. */
	 type = GetTxOperandType(pl,operand);	/* Get operand type.	*/
	 break;
	} /* END OF for(ALLOP(pl,operand)) */
 if(type == Tnone)				/* If no displacement is */
	goto END235;				/* from R, forget it.	*/
 if(!IsAdDisp(an_id))				/* If not Displacement mode, */
	goto END235;				/* forget it.	*/
 if(IsFP(type))					/* If floating-point operand,*/
	goto END235;				/* forget it.	*/
 bt = TySize(type);
 bi = GetAdNumber(Tbyte,adf1);
 if((bt != bi) && (bt != -bi))
	goto END235;
 if(!IsAdNumber(an_id))				/* If displacement not a pure */
	goto END235;				/* number, forget it.	*/
 if(Pskip(PEEP235))				/* Do we want to do this? */
	goto END235;				/* No.	*/
 displacement = GetAdNumber(type,an_id);	/* If it is, get it.	*/
 regid = GetAdRegA(adf3);
 if(opf == G_ADD3)				/* Handle G_ADD3 case.	*/
	{if(bi < 0)
		{if(displacement == 0)		/* Case 1.	*/
			{an_id = GetAdPreDecr(type,"",regid);
			}
		 else if(displacement == bi)	/* Case 3.	*/
			{an_id = GetAdPostDecr(type,"",regid);
			}
		 else
			goto END235;
		} /* END OF if(bi < 0) */
	 else if(bi > 0)
		{if(displacement == 0)		/* Case 2.	*/
			{an_id = GetAdPreIncr(type,"",regid);
			}
		 else if(displacement == -bi)	/* Case 4.	*/
			{an_id = GetAdPostIncr(type,"",regid);
			}
		 else
			goto END235;
		} /* END OF else if(bi > 0) */
	 else
		goto END235;
	} /* END OF if(opf == G_ADD3) */
 else if(opf == G_SUB3)				/* Handle G_SUB3 case.	*/
	{if(bi > 0)
		{if(displacement == 0)		/* Case 5.	*/
			{an_id = GetAdPreDecr(type,"",regid);
			}
		 else if(displacement == bi)	/* Case 7.	*/
			{an_id = GetAdPostDecr(type,"",regid);
			}
		 else
			goto END235;
		} /* END OF if(bi > 0) */
	 else if(bi < 0)
		{if(displacement == 0)		/* Case 6.	*/
			{an_id = GetAdPreIncr(type,"",regid);
			}
		 else if(displacement == -bi)	/* Case 8.	*/
			{an_id = GetAdPostIncr(type,"",regid);
			}
		 else
			goto END235;
		} /* END OF if(bi < 0) */
	 else
		goto END235;
	} /* END OF else if(opf == GSUB3) */
 else
	goto END235;				/* Not G_ADD3 or G_SUB3. */
 peepchange("use auto-address modes preceeding",peeppass,235);
 PutTxOperandAd(pl,operand,an_id);		/* Replace address of operand */
						/* in second instruction. */
 DelTxNode(pf);					/* Delete G_ADD3 or G_SUB3. */
 ndisc += 1;					/* Update number discarded. */
 return(TRUE);					/* (We did it.)	*/
END235:
#endif
/************************************************************************/
/*									*/
/************************** Register Propagation ************************/
/*									*/
/* 241 - Use move source register if possible as source for generic	*/
/*									*/
/*	G_MOV   &0  &0  R  B	->	G_MOV   &0  &0  R  B		*/
/*	G_op    C   D   E  F	->	G_op    C   D   R  F		*/
/*									*/
/*	if E is not a register						*/
/* or									*/
/*									*/
/*	G_MOV   &0  &0  R  B	->	G_MOV   &0  &0  R  B		*/
/*	G_op    C   E   D  F	->	G_op    C   R   D  F		*/
/*									*/
/*	if E is not a register						*/
/*									*/
/*	Requirements added to support WE32200:				*/
/*									*/
/*	    1.)	B must not be auto-address				*/
/*	    2.)	if B is index-register-with-scaling,			*/
/*		data size must be the same as E.			*/
/*									*/
/************************************************************************/

 if(!IsOpGMove(opf))			/* first is a move */
	goto END241;
 if(!IsAdCPUReg(adf2))			/* R is a register */
	goto END241;
 if(IsAdCPUReg(adf3))			/* were R is propagating to is not */
					/* a register */
	goto END241;

#ifdef w32200
 if(IsAdAutoAddress(adf3))		/* If B is auto-address, keepit so */
	goto END241;			/* we diddle the register.	*/
#endif

 if(!IsOpGeneric(opl))			/* second is a generic */
	goto END241;
 if(opl == G_MOVA)			/* Second is not MOVA or PUSHA. */
	goto END241;
 if(opl == G_PUSHA)
	goto END241;

#ifdef W32200
 if(AutoDepend)					/* We don't want E to	*/
	goto END241;				/* auto-depend on C or D. */
#endif

 f = (boolean) ((adf3 == adl2) && chktyp(pf,2,pl,2,&temp,&tmpaddr));
 if(!f && !(adf3 == adl1 && chktyp(pf, 2, pl, 1, &temp, &tmpaddr)))
					/* B == E and types are okay */
	goto END241;

#ifdef W32200
 if(IsAdIndexRegScaling(adf3))		/* If index-register-with-scaling, */
	{if(f)				/* make sure sizes are the same. */
		{if(szf3 != szl2)
			goto END241;		/* Not the same.	*/
		}
	 else
		{if(szf3 != szl1)
			goto END241;		/* Not the same.	*/
		}
	} /* END OF if(IsAdIndexRegScaling(adf3)) */
#endif

 if(!IsAdSafe(adf3) || IsTxOperandVol(pf,3)) 	/* non-volatile */
	goto END241;
 if(!legalgen((unsigned short) opl,tyl0,GetAdMode(adl0),
			((f) ? tyl1 : temp),
				((f) ? GetAdMode(adl1) : GetAdMode(tmpaddr)),
			((f) ? temp : tyl2),
				((f) ? GetAdMode(tmpaddr) : GetAdMode(adl2)),
			tyl3, GetAdMode(adl3) )  /* creating something legal */
	 )
	goto END241;
 if(Pskip(PEEP241))
	goto END241;
	{
	 peepchange( "use move src register as source of generic" ,peeppass,241);
	 PutTxOperandAd(pl,(unsigned int) ((f) ? 2 : 1),tmpaddr);
	 PutTxOperandType(pl,(unsigned int) ((f) ? 2 : 1),temp);
	 return(TRUE);
	} /* end of use move src register, if possible, as source in generic */
 END241:
/************************************************************************/
/*									*/
/* 242 - This transformation propagates a register use back into a generic.*/
/*									*/
/*	G_op   &0  O1  O2  O3	->	G_op   &0  O1  O2  R		*/
/*	G_MOV  &0  &0  O3  R	->	G_MOV  &0  &0  R   O3		*/
/*									*/
/*	if R not used by O3						*/
/*	O3 must not be auto-address mode. If O3 is indexed-register-with*/
/*	scaling, size of data item must be the same in both instructions*/
/*	(This is taken care of by the requirement that szf3 and szl2	*/
/*	be full words.)							*/
/*									*/
/*	If O3 is a register that is dead after movw, this is a 		*/
/*	special case of merging moves.  If O3 is a register that 	*/
/*	is not dead, we can't do the transformation, because it 	*/
/*	will oscillate.  Therefore, consider only O3 not a register.	*/
/*									*/
/************************************************************************/

 if(!IsOpGMove(opl))			/* second is a move */
	goto END242;
 if(adf3 != adl2)			/* O3 == O3 */
	goto END242;
 if(!IsAdCPUReg(adl3))			/* R is a register */
	goto END242;
 if(!IsOpGeneric(opf))			/* 1st is a generic */
	goto END242;
 if(!IsAdSafe(adf3) || IsTxOperandVol(pf,3))	/* nonvolatile */
	goto END242;
 if(IsAdCPUReg(adf3))			/* O3 is not a register */
	goto END242;

#ifdef W32200
 if(IsAdAutoAddress(adf3))		/* O3 must not be auto-address.	*/
	goto END242;
 if(AutoDepend)					/* O3 must not be auto-	*/
	goto END242;				/* dependent on O1 or O2. */
#endif

 if(IsOpDstSrc(opf))			/* O3 must be a pure dest */
	goto END242;
					/* In order for O3 to not be */
					/* truncating the data going to R, */
					/* sz(O3) must be szwd. */
 if(szf3 != szwd)
	goto END242;
 if(szl2 != szwd)
	goto END242;
 if(!isdeadcc(pl) && szl3 != szwd)
					/* If the condition codes are live, */
				   	/* then R must be the same size as */
				   	/* O3, ie., == szwd. */
	goto END242;
 if(IsAdImmediate(adl2))		/* can't be an immediate due to */
					/* swap to dest */
	goto END242;
	  				/* R not used by O3 */
 if(!notaffected(adf3,adl3))
	goto END242;
 if(!legalgen((unsigned short) opf,
		tyf0,GetAdMode(adf0),tyf1,GetAdMode(adf1),
		tyf2,GetAdMode(adf2),tyf3,GetAdMode(adl3)))
	goto END242;
 temp = islivecc(pl) ? tyf3 : tyl3;
 if(!legalgen((unsigned short) opl,
		tyl0,GetAdMode(adl0),tyl1,GetAdMode(adl1),
		tyl2,GetAdMode(adl3),temp,GetAdMode(adl2)))
	goto END242;
 if(Pskip(PEEP242))
	goto END242;
	{
	 peepchange("use register as a destination",peeppass,242);
	 PutTxOperandAd(pf,DESTINATION,adl3);
	 PutTxOperandAd(pl,DESTINATION,adl2);
	 PutTxOperandType(pl,DESTINATION,temp);
	 PutTxOperandAd(pl,2,adl3);
	 return(TRUE);
	} /* end of propagating R back to O3 */
 END242:
/************************************************************************/
/*									*/
/* 244 - Use move destination register,					*/
/*	if possible, as source for a generic.				*/
/*									*/
/* This improvement propagates the use of a register, if possible.	*/
/* It also has the potential of killing off the liveness of a register.	*/
/*									*/
/* Example:								*/
/*									*/
/*	G_MOV   &0  &0  A  R	->	G_MOV   &0  &0  A  R		*/
/*	G_op    C   D   E  F	->	G_op    C   D   R  F		*/
/*									*/
/*	G_MOV   &0  &0  A  R	->	G_MOV   &0  &0  A  R		*/
/*	G_op    C   E   D  F	->	G_op    C   R   D  F		*/
/*									*/
/*		where A has same address as E				*/
/*		A must not be an auto-address mode.			*/
/*		If A is Indexed-Register-with-Scaling mode,		*/
/*	the size of operand A must be the same as the size of operand E.*/
/*									*/
/*	Since we are trying to kill registers off early, these special	*/
/*	conditions apply if A is a register:				*/
/*		1)  A must be dead after second instruction 		*/
/*			or be the same as F.				*/
/*		2)  R must be live after second instruction.		*/
/*	A may not use R.						*/
/*									*/
/*	Should not be done if G_op is move and A is &0 			*/
/*		because 32100 has a CLR instruction.			*/
/*									*/
/************************************************************************/

						/* In what follows, the	*/
						/* goto's are executed	*/
						/* whenever a condition	*/
						/* required for the	*/
						/* optimization is	*/
						/* unsatisfied.		*/
 if(!IsOpGMove(opf))				/* 1st instr must be move. */
	goto END244;
 if(!IsAdCPUReg(adf3))				/* G_MOV must have reg dest. */
	goto END244;
 if(		(opl == G_MOV)			/* G_op must not be a */
	     && IsAdImmediate(adl2)		/* CLR: a G_MOV with a source */
	     && IsAdNumber(adl2)		/* whose value is an */
	     && (GetAdNumber(tyl2,adl2) == 0))	/* immediate 0.	*/
	goto END244;
 if(adf2 == i0)					/* If source of first	*/
	goto END244;				/* instruction is &0,	*/
						/* don't do this because */
						/* it will pick up unused */
						/* operands and because it */
						/* could wipe out other	*/
						/* optimizations.	*/
 if(!IsOpGeneric(opl))				/* 1st instr must be generic. */
	goto END244;

#ifdef W32200
 if(IsAdAutoAddress(adf2))			/* A must not be AutoAddress. */
	goto END244;				/* It is if here.	*/
 if(AutoDepend)					/* We don't dare mess around */
	goto END244;				/* if auto-dependencies. */
#endif

 f = (boolean)	(adf2 == adl2 && chktyp(pf,DESTINATION,pl,2,&temp,&tmpaddr));
						/* Flag true for first case. */
 if( !f && !(adf2 == adl1 && chktyp(pf,DESTINATION,pl,1,&temp,&tmpaddr)))
						/* If not first case, */
						/* it is second case. */
	goto END244;

#ifdef W32200
 if(IsAdIndexRegScaling(adf2))			/* If Indexed-Register-with- */
	{if(f)					/* scaling, be sure sizes are*/
		{if(szf2 != szl2)
			goto END244;		/* the same: not if here. */
		}
	 else
		{if(szf2 != szl1)
			goto END244;
		}
	} /* END OF if(IsAdIndexRegScaling(adf2)) */
#endif

 if(IsAdCPUReg(adf2) && !(IsDeadAd(adf2,pl) || adf2 == adl3))
						/* If A is a register, */
						/* it must be dead after G_op */
						/* or equal to dst of G_op. */
	goto END244;
 if(IsAdCPUReg(adf2) && adl2 == adl3 && f)	/* If A is a register, */
						/* OP must not be a dyadic, */
						/* but only in the first case. */
	goto END244;
 if(IsAdCPUReg(adf2) && IsDeadAd(adf3,pl))	/* If A is a register, */
						/* R must be live after G_op. */
	goto END244;
 if(opl == G_MOVA)				/* G_op must not be G_MOVA. */
	goto END244;
 if(opl == G_PUSHA)				/* G_op must not be G_PUSHA. */
	goto END244;
 if(!notaffected(adf2,adf3))			/* E must not use R. */
	goto END244;
 /*
  * we assume that "A is not volatile but E is" can only happen if
  * 1) A is n(R), but since A can't use R, this can't happen
  * 2) A and E are members of a union with one a volatile and
  *	the other not. this is ill-defined.
  * 3) one of A or E is a cast to volatile and the other isn't.
  *	this is the same case as 2)
  */
 if(!IsAdSafe(adf2) || IsTxOperandVol(pf,2))	/* E must not be volatile. */
	goto END244;
 if(!legalgen((unsigned short) opl,		/* Newone must be legal	*/
		tyl0,				/* generic.	*/
		GetAdMode(adl0),
		((f) ? tyl1 : temp),
		((f) ? GetAdMode(adl1) : GetAdMode(tmpaddr)),
		((f) ? temp : tyl2),
		((f) ? GetAdMode(tmpaddr) : GetAdMode(adl2)),
		tyl3,
		GetAdMode(adl3)))
	goto END244;
 if(Pskip(PEEP244))
	goto END244;
						/* If here, it is OK to	*/
						/* do the optimization.	*/
	{
 	 peepchange("use move dst register as src for generic",peeppass,244);
 	 PutTxOperandAd(pl,(unsigned int) ((f) ? 2 : 1),tmpaddr);
 	 PutTxOperandType(pl,(unsigned int) ((f) ? 2 : 1),temp);
 	 return(TRUE);
	}
END244:		/* END OF using move dst register as src for generic.	*/
/************************************************************************/
/*									*/
/* 245 - Re-order pairs of instructions to make better use of registers.*/
/*									*/
/* this transformation reverses a operation and a			*/
/* G_MOV to make better use of registers:				*/
/*									*/
/*	G_op   &0  O1  O2  O3   ->	G_MOV   &0  &0  O2  R		*/
/*	G_MOV  &0  &0  O2  R	->	G_op    &0  O1  R  O3		*/
/*									*/
/*	G_op   &0  O1  O2  O3   ->	G_MOV   &0  &0  O1  R		*/
/*	G_MOV  &0  &0  O1  R	->	G_op    &0  R   02  O3		*/
/*									*/
/* That is, it inserts a register reference into G_op.			*/
/* R must not be used in O1, O2, or O3.					*/
/* O3 cannot be the same as R, nor can O1/O2 use O3 in any way.		*/
/* Condition codes cannot be live after move, since we're		*/
/* setting different condition codes.					*/
/* Don't bother if R dead after move, since the inst will  		*/
/* go away eventually.							*/
/*									*/
/*	Additional requirements imposed to support WE32200:		*/
/*									*/
/*	    1.)	adl2 must not be auto-address mode.			*/
/*	    2.)	if O3 is auto-address mode, its register may not be used*/
/*		by adl2.						*/
/*	    3.)	if adl2 is indexed-register-with-scaling mode, the sizes*/
/*		of adl2 and (O1 or O2 [as appropriate]) must be the	*/
/*		same.							*/
/*									*/
/************************************************************************/

 if(IsOpGMove(opf))			/* don't want to undo move */
					/* optimizations */
	goto END245;
 if(!IsOpGMove(opl))			/* 2nd instruction is cpu move */
	goto END245;
 if(!IsOpGeneric(opf))			/* 1st is a generic */
	goto END245;
 if(!IsAdCPUReg(adl3))			/* R is a register */
	goto END245;
 if(opf == G_MOVA) 
	goto END245;

#ifdef W32200
 if(AutoDepend)					/* AutoDependencies too	*/
	goto END245;				/* confusing.	*/
#endif

 f = (boolean) ((adl2 == adf2) && chktyp(pl,DESTINATION,pf,2,&temp,&tmpaddr));
				/* O2 == O2 and types are okay */
 if(!f && !((adl2 == adf1) && chktyp(pl, DESTINATION, pf, 1, &temp, &tmpaddr)))
				/* O1 == O1 and types are okay */
	goto END245;
 if(IsOpDstSrc(opf))			/* O3 is not the source */
	goto END245;
 if(opf == G_PUSHA)
	goto END245;

#ifdef W32200
 if(IsAdAutoAddress(adl2))		/* adl3 must not be auto-address. */
	goto END245;			/* It is if here.	*/
 if(IsAdIndexRegScaling(adf2))		/* Sizes must agree if this mode. */
	{if(f)
		{if(szl2 != szf2)
			goto END245;
		}
	 else
		{if(szl2 != szf1)
			goto END245;
		}
	} /* END OF if(IsAdIndexRegScaling(adf2)) */
#endif

 if(IsDeadAd(adl3, pl))
	goto END245;
 if(IsAdCPUReg(adl2))
	goto END245;
 /*
  * we assume that "A in one instr is not volatile but is in the next"
  * can only happen if
  * 1) A uses 03, but since A can't use 03, this can't happen
  * 2) both A's are members of a union with one a volatile and
  *	the other not. this is ill-defined.
  * 3) one of the A's is a cast to volatile and the other isn't.
  *	this is the same case as 2)
  */
 if(!IsAdSafe(adl2) || IsTxOperandVol(pl,2))	/* O2 must be nonvolatile */
	goto END245;
 if(	(f && IsTxOperandVol(pf,1)) ||		/* no volatile refs in pf */
	(!f && IsTxOperandVol(pf,2)) ||
	IsTxOperandVol(pf,3)
   )
	goto END245;
 if(!notaffected(adl2,adf3))		/* O1/O2 doesn't use O3 */
	goto END245;
 if(!notaffected(adf0,adl3))		/* operands of G_op don't use R */
	goto END245;
 if(!notaffected(adf1,adl3))
	goto END245;
 if(!notaffected(adf2,adl3))
	goto END245;
 if(!notaffected(adf3,adl3))
	goto END245;
 if(!isdeadcc(pl))	/* the condition codes are not alive */
	goto END245;
 if(!legalgen((unsigned short) opf,tyf0,GetAdMode(adf0),
		((f) ? tyf1 : temp),
			((f) ? GetAdMode(adl1) : GetAdMode(tmpaddr)),
		((f) ? temp : tyf2),
			((f) ? GetAdMode(tmpaddr) : GetAdMode(adf2)),
		tyf3,GetAdMode(adf3)))
	goto END245;
 if(Pskip(PEEP245))
	goto END245;
	{
	 peepchange( "reorder instructions to use registers" ,peeppass,245);
	 lexchin( pf, pl );
	 (void) MoveTxNodeAfter(pf,pl);		/* Now pf and pl are flipped */
						/* but not preset variables; */
						/* i.e., adl3.	*/
	 PutTxOperandAd(pf,
		(unsigned int) ((f) ? 2 : 1),tmpaddr);	/* O1 is set to R */
	 PutTxOperandType(pf,(unsigned int) ((f) ? 2 : 1),temp);
	 return(TRUE);
	} /* end of reordering an operation and a move to better use register */
 END245:
/************************************************************************/
/*									*/
/* 246 - Merge successive adds.						*/
/*									*/
/* This improvement occurs in address arithmetic and in some funny	*/
/* expressions.								*/
/*									*/
/*	G_ADD3   &0  X   Y   R1   ->   G_ADD3   &0  X  R2  R2		*/
/*	G_ADD3   &0  R1  R2  R2   ->   G_ADD3   &0  Y  R2  R2		*/
/*									*/
/*	R1 dead after second G_ADD3 					*/
/*	R1 and R2 are different						*/
/*	Y can not use R2						*/
/*									*/
/* Unfortunately, we learn the hard way of special cases we hadn't	*/
/* thought of earlier.  If Y is R2, we can't do this at all!		*/
/*									*/
/*	Requirements added to support WE32200:				*/
/*	    1.)	If X is auto-address mode, it must not use R1 or R2.	*/
/*	    1a.) If X is auto-address mode, Y may not use its register.	*/
/*	    2.)	If Y is auto-address mode, it must not use R1 or R2.	*/
/*									*/
/************************************************************************/

 if(opf != G_ADD3)
	goto END246;
 if(opl != G_ADD3)			/* have 2 G_ADD */
	goto END246;
 if(adl1 != adf3)			/* R1 == R1 */
	goto END246;
 if(adl2 != adl3)			/* second one is a dyadic */
	goto END246;
 if(tyl2 != tyl3)
	goto END246;
 if(szl1 < szl3)
					/* R1 in second instruction must not */
				   	/* be truncating the data */
	goto END246;
 if(!IsAdCPUReg(adf3))			/* R1 is a register */
	goto END246;
 if(!IsAdCPUReg(adl3))			/* R2 is a register */
	goto END246;

#ifdef W32200
 if(AutoDepend)					/* Autodependency test is */
	goto END246;				/* sufficient test.	*/
#endif

 if(!IsDeadAd(adf3,pl))			/* R1 dead following second add */
	goto END246;
 if(adf3 == adl3)			/* R1 != R2 */
	  				/* Y does not use R1 */
	goto END246;
 if(!notaffected(adf2,adl3))
	goto END246;
 if(Pskip(PEEP246))
	goto END246;
	{
	 peepchange( "merge successive adds" ,peeppass,246);
	 PutTxOperandAd(pl,1,adf2);
	 PutTxOperandType(pl,1,tyf2);
	 PutTxOperandAd(pf,DESTINATION,adl3);
	 PutTxOperandType(pf,DESTINATION,tyl3);
	 PutTxOperandAd(pf,2,adl2);
	 PutTxOperandType(pf,2,tyl2);
	 return(TRUE);
	} /* end of merge succesive adds */
 END246:
	return( FALSE );
}	
