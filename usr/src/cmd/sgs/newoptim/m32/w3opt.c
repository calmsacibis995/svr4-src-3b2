/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)newoptim:m32/w3opt.c	1.7"

/****************************************************************/
/*				w3opt.c				*/
/*								*/
/*		32100 three-instruction window improver		*/
/*								*/
/*	This module contains improvements for three instruction	*/
/*	windows, of which there aren't many.			*/
/*								*/
/*								*/
/****************************************************************/

/****************************************************************/
/* Note: The use of goto's in this file instead of long strings */
/* if "if else" clauses is done to increase portability.	*/
/****************************************************************/

#include	<stdio.h>
#include	"defs.h"
#include	"debug.h"
#include	"ANodeTypes.h"
#include	"olddefs.h"
#include	"OpTabTypes.h"
#include	"OpTabDefs.h"
#include	"RegId.h"
#include	"RegIdDefs.h"
#include	"ANodeDefs.h"
#include	"OperndType.h"
#include	"LoopTypes.h"
#include	"RoundModes.h"
#include	"TNodeTypes.h"
#include	"TNodeDefs.h"
#include	"optab.h"
#include	"optutil.h"
#include	"Target.h"

#define	isdispc(a)	(boolean) (IsAdDisp(a) || IsAdDispDef(a))


	boolean				/* TRUE if changes made.	*/
w3opt(pf,pl,peeppass)
register TN_Id pf;			/* TN_Id of first inst. in window. */
register TN_Id pl;			/* TN_Id of last inst. in window. */
unsigned int peeppass;

{
 AN_Id adf2;
 AN_Id adf3;
 AN_Id adm1;
 AN_Id adm2;
 AN_Id adm3;
 AN_Id adl2;
 AN_Id adl3;			/* AN_Id of last operand of last instruction. */
 extern enum CC_Mode ccmode;	
 boolean f;
 extern void lexchin();		/* Exchange instructions (line numbers). */
 extern void peepchange();	/* Local version of wchange.	*/
 unsigned int opf;		/* op code index of first instruction. */
 unsigned int opm;		/* op code index of second instruction. */
 unsigned int opl;		/* op code index of third  instruction. */
 register TN_Id pm;		/* TN_Id of middle node.	*/
 extern boolean setslivecc();	/* True if instruction sets live CC's.	*/
 OperandType tyf2; 
 OperandType tym2; 
 OperandType tym3; 
 OperandType tyl2; 
 OperandType tyl3; 		/* Type of last operand of last instruction. */

#ifdef W32200
 extern boolean IsAutoAddress();	/* TRUE if auto-address in window. */
 extern boolean IsAutoDependency();	/* TRUE if autodependency in window. */
 boolean AutoAddress;		/* TRUE if auto-address in window.	*/
 boolean AutoDepend;		/* TRUE if auto-dependency in window.	*/
 extern unsigned int Max_Ops;	/* Maximum number of operands in an instr. */
 AN_Id adf0;			/*AN_Id of first operand of first instruction.*/
 AN_Id adf1;
 AN_Id adm0;			/*AN_Id of first operand of middle instr. */
 AN_Id adl0;			/*AN_Id of first operand of middle instr. */
 AN_Id adl1;
 AN_Id an_id;			/* AN_Id of new address node.	*/
 AN_Id i2;			/* Address of immediate 2.	*/
 extern unsigned int ndisc;	/* Number of instructions discarded.	*/
 unsigned int operand;		/* Operand counter.	*/
 unsigned int regno;		/* Register number for 310.	*/
 OperandType tyf1; 		/* Type of 2nd operand of first instruction. */
 OperandType type;		/* Type of unspecified operand.	*/
 OperandType tyf3; 
#endif
						/* INITIALIZE.	*/
 pm = GetTxNextNode(pf);			/* Get TN_Id of middle node. */
 adf2 = GetTxOperandAd(pf,2);
 adf3 = GetTxOperandAd(pf,3);
 adm1 = GetTxOperandAd(pm,1);
 adm2 = GetTxOperandAd(pm,2);
 adm3 = GetTxOperandAd(pm,3);
 adl2 = GetTxOperandAd(pl,2);
 adl3 = GetTxOperandAd(pl,3);
 opf = GetTxOpCodeX(pf);			/* Get op-code index of	*/
 opm = GetTxOpCodeX(pm);			/* the three	 */
 opl = GetTxOpCodeX(pl);			/* instructions.	*/
 tyf2 = GetTxOperandType(pf,2); 
 tym2 = GetTxOperandType(pm,2); 
 tym3 = GetTxOperandType(pm,3); 
 tyl2 = GetTxOperandType(pl,2); 
 tyl3 = GetTxOperandType(pl,3); 

#ifdef W32200
 adf0 = GetTxOperandAd(pf,0);	
 adf1 = GetTxOperandAd(pf,1);
 adm0 = GetTxOperandAd(pm,0);
 adl0 = GetTxOperandAd(pl,0);
 adl1 = GetTxOperandAd(pl,1);
 tyf1 = GetTxOperandType(pf,1); 
 tyf3 = GetTxOperandType(pf,3); 

 AutoAddress = IsAutoAddress(pf,pl);		/* Check for autoaddressing. */
 AutoDepend = (AutoAddress
		&& IsAutoDependency(	adf0,adf1,adf2,adf3,
					adm0,adm1,adm2,adm3,
					adl0,adl1,adl2,adl3,NULL)) ?
			TRUE : FALSE;
#endif

		/* there is a possiblity of looping with PEEP#222 */
 if(ccmode != Transition && peeppass >= 2)
	return( FALSE );
/************************************************************************/
/*									*/
/* 300 -  address arithmetic: propagating increments			*/
/*									*/
/*	G_MOV   0  0  A  R	->	G_MOV   0  0  A  R		*/
/*	G_ADD3  0  &n A  A	->	G_move	0  0  0(R) B		*/
/*	G_move  0  0  0(R) B	->	G_ADD3  0  &n A  A		*/
/*									*/
/*				or					*/
/*									*/
/*	G_MOV   0  0  A  R	->	G_MOV   0  0  A  R		*/
/*	G_ADD3  0  &n A  A	->	G_move	0  0  B  0(R)		*/
/*	G_move  0  0  B  0(R)	->	G_ADD3  0  &n A  A		*/
/*									*/
/*		where B does not use A in any way,			*/
/*		A does not use R in any way,				*/
/*		in first case, B is not used by A, and			*/
/*		the last move is not setting needed condition codes	*/
/*									*/
/* 	Note that B can use or set R without problems.			*/
/*									*/
/*      Note that this optimization also depends on the assumption	*/
/*	that A does not contain its own address.  This is		*/
/*	guaranteed only in the case where A is a NAQ.			*/
/*	If A is not a NAQ, A can contain its own address, but the	*/
/*	source code required to create the situation is very unusual.	*/
/*	For example, the portable C compiler generates such code for	*/
/*	the source code:						*/
/*									*/
/*		int i, j;						*/
/*									*/
/*		i = (int) &i;						*/
/*		j = *(int *)i++;					*/
/*									*/
/*	The above example is considered to be undefined in C.		*/
/*	Therefore, for this C example, it is considered acceptable for the  */
/*	optimizer to change the meaning of the program.			*/
/*									*/
/*	It is not known whether this is true for other languages.	*/
/*									*/
/************************************************************************/

 if(opf != G_MOV)			/* First opcode must be CPU move, and */
	goto END300;			/* (it isn't if here)	*/
 if(opm != G_ADD3)			/* Second opcode must be CPU add, and */
	goto END300;			/* (it isn't if  here)	*/
 if(!IsOpGMove(opl))			/* Third opcode must be generic move, */
	goto END300;			/* and (it isn't if here)	*/
 if(!IsAdCPUReg(adf3))			/* R must be a CPU register, and */
	goto END300;			/* (it isn't if here)	*/
 if(!IsAdImmediate(adm1))		/* n must be immediate	*/
	goto END300;			/* (it isn't if here)	*/
 if(!IsAdNumber(adm1))			/* constant, and	*/
	goto END300;			/* (it isn't if here) 	*/
 if(adf2 != adm2)
	goto END300;
 if(adm2 != adm3)			/* A == A == A	*/
	goto END300;
 if(tyf2 != tym2)
	goto END300;
 if(tym2 != tym3)
	goto END300;

#ifdef W32200
 if(AutoDepend)					/* Autodependencies are	*/
	goto END300;				/* too risky.	*/
#endif

 f = (boolean) (isdispc(adl2)	&&		/* First case.	*/
		(GetAdNumber(tyl2,adl2) == 0) && 
		IsAdUses(adl2,adf3)
	       );
 if(	!f &&
	!(isdispc(adl3)	&&			/* Second case.	*/
		 (GetAdNumber(tyl3,adl3) == 0) && 
		 IsAdUses(adl3,adf3)
	 )
   )
	goto END300;			/* 0(R) is indirect off from R.	*/
 if(!notaffected(((f) ? adl3 : adl2),adf2))	/* B must not use A	*/
	goto END300;				/* (it does, if here)	*/
 if(!notaffected(adf2,adf3))			/* A must not use R	*/
	goto END300;				/* (it does, if here)	*/
 if((f) && !notaffected(adf2,adl3))		/* In first case, A does */
						/* not use B	*/
	goto END300;
 if(setslivecc(pl))				/* Condition codes of last */
	goto END300;				/* move must not be needed */
 /*
  * Note that we need not check volatility in the second instruction.
  * we assume that "A in one instr is not volatile but is in the next" 
  * can only happen if
  * 1) A is n(R), but since A can't use R, this can't happen
  * 2) both A's are members of a union with one a volatile and
  *	the other not. this is ill-defined.
  * 3) one of A's is a cast to volatile and the other isn't.
  *	this is the same case as 2)
  */
 if(!IsAdSafe(adf2) || IsTxOperandVol(pf,2))	/* A must not be volatile. */
	goto END300;				
 if(IsTxOperandVol(pl,2) ||			/* No volatile references in */
	IsTxOperandVol(pl,3))			/* last instruction. */
		goto END300;
 if(Pskip(PEEP300))				/*If we don't want to do this,*/
	goto END300;				/* skip it.	*/
 peepchange("propagate increments",peeppass,300);
 lexchin(pm,pl);
 (void) MoveTxNodeAfter(pm,pl);
 return(TRUE);

END300:

#ifdef W32200
/************************************************************************/
/*									*/
/* 310 - try to use indexed-register-with-scaling mode if we32200 CPU.	*/
/*									*/
/*	Requirements on new peephole 310:				*/
/*									*/
/*	G_ADD3	&0	R1	R1	R3				*/
/*	G_ADD3	&0	R2	R3	R3				*/
/*	G_op	...t 0(R3)...		->	G_op	...t R2[R1]...	*/
/*									*/
/*	where t is Thalf or Tuhalf					*/
/*									*/
/*	G_ALS3	&0	&2	R1	R3				*/
/*	G_ADD3	&0	R2	R3	R3				*/
/*	G_op	...t 0(R3)...		->	G_op	...t R2[R1]...	*/
/* 									*/
/*	G_LLS3	&0	&2	R1	R3				*/
/*	G_ADD3	&0	R2	R3	R3				*/
/*	G_op	...t 0(R3)...			->	G_op	...t R2[R1]...
/*									*/
/*	where t is Tword or Tuword					*/
/*									*/
/*	and where, for both cases,					*/
/*		R1 is in first register set.				*/
/*		R2 is in second register set.				*/
/*		R3 is dead after G_op.					*/
/*		All operands of G_ADD3, G_ALS3, and G_LLS3 are Tword or Tuword.
/*		Operand of G_op is not floating point.			*/
/*									*/
/************************************************************************/
 if(cpu_chip != we32200)			/* If not we32200,	*/
	goto END310;				/* don't use we32200 modes. */
 if(!IsOpGeneric(opl))				/* If last operator not	*/
	goto END310;				/* generic, forget it.	*/
 if(opm != G_ADD3)				/* If middle operator not */
	goto END310;				/* G_ADD3, forget it.	*/
 if((opf != G_ADD3) && (opf != G_ALS3) && (opf != G_LLS3))	/* If first */
	goto END310;				/* operator not oppropriate, */
						/* forget it.	*/
 if(!IsDeadAd(adf3,pl))				/* R3 must be dead.	*/
	goto END310;
 if(!IsAdCPUReg(adf3))				/* "Third" operand of first */
	goto END310;				/* instruction must be a */
						/* CPU register.	*/
 if(adm2 != adf3)				/* Establish register use */
	goto END310;				/* patterns.	*/
 if(adm3 != adf3)
	goto END310;
 if(!IsAdCPUReg(adf2))				/* "Second" operand of first */
	goto END310;				/* instruction must be */
 regno = GetRegNo(GetAdRegA(adf2));		/* a CPU register in first */
 if(regno > GetRegNo(CREG15))			/* register set.	*/
	goto END310;				/* Isn't if here.	*/
 if(!IsAdCPUReg(adm1))				/* "First" operand of second */
	goto END310;				/* instruction must be */
 regno = GetRegNo(GetAdRegA(adm1));		/* a CPU register in second */
 if(regno < GetRegNo(CREG16))			/* register set.	*/
	goto END310;				/* Isn't if here.	*/
 if((tyf1 != Tword) && (tyf1 != Tuword))	/* All operands of first */
	goto END310;				/* must be Tword or Tuword.*/
 if((tyf2 != Tword) && (tyf2 != Tuword))	/* All operands of first */
	goto END310;				/* must be Tword or Tuword.*/
 if((tyf3 != Tword) && (tyf3 != Tuword))	/* All operands of first */
	goto END310;				/* must be Tword or Tuword.*/
 if(Pskip(PEEP310))				/* Do we want to do this? */
	goto END310;				/* No.	*/
 type = Tnone;					/* We don't know type yet. */
 for(ALLOP(pl,operand))				/* Examine all operands. */
	{an_id = GetTxOperandAd(pl,operand);	/* Get operand address.	*/
	 if(!IsAdUses(an_id,adf3))		/* Does it use the register? */
		continue;			/* No: try next operand. */
	 type = GetTxOperandType(pl,operand);	/* Get operand type.	*/
	 break;
	} /* END OF for(ALLOP(pl,operand)) */
 if(type == Tnone)				/* If no displacement is */
	goto END310;				/* from R, forget it.	*/
 if(!IsAdDisp(an_id))				/* If not Displacement mode, */
	goto END310;				/* forget it.	*/
 if(IsFP(type))					/* If floating-point operand,*/
	goto END310;				/* forget it.	*/
 i2 = GetAdImmediate("2");
 if(opf == G_ADD3)				/* Might be case 1.	*/
	{if((type != Thalf) && (type != Tuhalf))
		goto END310;			/* Type wrong: not case 1. */
	}
 else if(adf1 == i2)				/* Might be case 2 or 3. */
	{if((type != Tword) && (type != Tuword))
		goto END310;			/*Type wrong: not case 2 or 3.*/
	 if((opf != G_ALS3) && (opf != G_LLS3))
		goto END310;			/*Opcode wrong:not case 2 | 3.*/
	}
 else
	goto END310;				/* None of above.	*/
 peepchange("use indexed-register-with-scaling mode",peeppass,310);	/* Do it. */
 DelTxNode(pf);					/* Delete unneeded nodes. */
 DelTxNode(pm);
 ndisc += 2;					/* Update number deleted. */
 an_id = GetAdIndexRegScaling(type,"",		/* Make new address.	*/
	GetAdRegA(adm1),GetAdRegA(adf2));
 PutTxOperandAd(pl,operand,an_id);		/* Replace it in instruction.*/
 return(TRUE);					/* We did it.	*/
END310:
#endif

 return(FALSE);
}

