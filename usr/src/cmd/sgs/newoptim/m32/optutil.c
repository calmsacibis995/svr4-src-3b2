/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)newoptim:m32/optutil.c	1.14"

/* optutil.c
**
**	Optimizer utilities.
**
**
** This module contains utility routines for the various peephole
** window optimizations.
*/

#include	<stdio.h>
#include	"defs.h"
#include	"OpTabTypes.h"
#include	"OpTabDefs.h"
#include	"RegId.h"
#include	"RegIdDefs.h"
#include	"ANodeTypes.h"
#include	"ANodeDefs.h"
#include	"LoopTypes.h"
#include	"OperndType.h"
#include	"RoundModes.h"
#include	"olddefs.h"
#include	"TNodeTypes.h"
#include	"TNodeDefs.h"
#include	"ALNodeType.h"
#include	"FNodeTypes.h"
#include	"FNodeDefs.h"
#include	"optab.h"
#include	"optim.h"
#include	"optutil.h"
#include	"Target.h"

#define	DESTINATION	3

	/** getbit -- get bit number for power of 2
	**
	** This routine tests whether an operand is a power of 2.  If not, it
	** returns -1.  Otherwise it returns a bit number, which is also the
	** exponent of 2.
	**/

	int
getbit(adf)
AN_Id adf;				/* address node id */
{
    int gbit = 0;			/* initialize bit number for later */
    long n;				/* numeric value of operand */

    if ( ! IsAdImmediate(adf) || ! IsAdNumber(adf) || 
	(n = GetAdNumber(Tnone,adf)) < 0 )
	return(-1);			/* check for numeric literal with
					** good value
					*/

/* This test really works, although it looks suspicious.  A power of two
** has only one bit set, so that value minus one has all of the bits to
** the right of the original bit set.  Anding them together gives zero.
** No other value yields zero.
*/

    if ( ( n & (n-1) ) != 0)
	return(-1);			/* not a power of 2 */

/* Shift right until the word is zero */

    while ((n = n>>1) != 0)
	gbit++;				/* bump bit number each time */

    return(gbit);			/* return bit number */
}
	void
updlive(pbefore,pafter,lastlive,NVectors)
TN_Id pbefore, pafter;
unsigned long int lastlive[];
unsigned int NVectors;		/* Number of vectors to use.	*/

{			/* updates live/dead information after optimization */
 register TN_Id p;
 extern void sets();		/* Determines what is set by instruction. */
 unsigned long int temps[NVECTORS];
 unsigned long int tempu[NVECTORS];
 unsigned long int u[NVECTORS];
 extern void uses();	/* Determines what is used by instruction. */
 register unsigned int vector;

 p = GetTxPrevNode(pafter);
 PutTxLive(p,lastlive,NVectors);

 while(p != pbefore)
	{if(p == (TN_Id) NULL || IsTxLabel(p))
		return;
	 sets(p,temps,NVectors);		/* Get sets information. */
	 uses(p,tempu,NVectors);		/* Get uses information. */
	 GetTxLive(p,u,NVectors);		/* Get live information. */
	 for(vector = 0; vector < NVectors; vector++)	/* Recompute	*/
		{u[vector] &= ~ temps[vector];	/* live information.	*/
		 u[vector] |= tempu[vector];
		}
	 p = GetTxPrevNode(p);			/* Put live information	*/
	 PutTxLive(p,u,NVectors);		/* into previous node.	*/
	}
 return;
}
	boolean
notaffected(a1,a2)	/* is reading a1 not affected by writing a2 */
AN_Id a1;
AN_Id a2; 

{if(IsAdImmediate(a1))
	return(TRUE);
 if((IsAdNAQ(a1) || IsAdSNAQ(a1) || IsAdNAQ(a2) || IsAdSNAQ(a2)) 
		&& !IsAdUses(a1,a2))
	return(TRUE);
 return FALSE;
}




	TN_Id
skipprof(pn)
TN_Id pn; 		/* Skip label, SAVE, and profiling code */

	/* label, SAVE, and profiling code is assumed to be of the form
	 *	
	 *	(0 or more nodes that are not saved)
	 *	save	...		(or SAVE ...)
	 *	G_MOV	...		(of MOVW ...)		(optional)
	 *	jsb	_mcount		(or JSB	_mcount)	(optional)
	 */

{extern void fatal();		/* Handles fatal erroes.	*/
 TN_Id p;

 while((GetTxOpCodeX(pn) != ISAVE) &&
		(GetTxOpCodeX(pn) != SAVE) && (pn != (TN_Id) NULL)) 
	pn = GetTxNextNode(pn);			/* Find SAVE */
 if(pn != (TN_Id) NULL)
	pn = GetTxNextNode(pn);			/* Skip SAVE */
 else
	fatal("skipprof: ran off end.\n");

  		/* skip profile, if exists */
 if((p = pn)->op == G_MOV || GetTxOpCodeX(p) == MOVW) {
   if(((p = GetTxNextNode(p))->op == IJSB || GetTxOpCodeX(p) == JSB) && 
		GetTxOperandAd(p,0) == GetAdAbsolute(Tbyte,"_mcount"))
	return(GetTxNextNode(p)); /* normal profiling code */
 }
 else { /* try for PIC profiling code */
   if(PIC_flag && (p->op == G_MOVA || p->op == MOVAW) &&
          ((p = GetTxNextNode(p))->op == IJSB || GetTxOpCodeX(p) == JSB)) 
      if (GetTxOperandAd(p,0) == GetAdDispDef("_mcount@GOT",CPC))
	  return(GetTxNextNode(p));
/* Note: the operand *_mcount@GOT was actually treated as displacement/
   deferred operand relative to the PC, when it was first entered. */
 }
 return(pn);
}
	boolean
chktyp( pm, indm, pg, indc, pty, pad ) /* check types for merge of preceding move*/

/* This routine checks the constraints on types for valid merge of
** an operand of a move into an operand of a following generic.
** If the merge is valid, it returns "TRUE" and sets new values of
** type and address in the locations indicated in its last two arguments.
** If the merge is not valid, it returns "FALSE" and returns the
** existing type and address.  
**
** The model is:
** 
**	G_move	&0  &0  A  B
**	G_op    ... C ...	->	G_op	... C' ...
** 
**	where address of B equals address of C
**
**	Let:	tyx = type of X
**		szx = size of X
**		snx = TRUE if X is signed
*/

TN_Id pm;		/* G_MOV node */
unsigned int indm;		/* index of operand to be merged (2 or 3) */
TN_Id pg;		/* G_op node */
unsigned int indc;	/* index of C, operand to merge into ( 0, 1, or 2 ) */
OperandType *pty;		/* pointer to new type */
AN_Id *pad;			/* Pointer to new address-node-identifier */

{extern AN_Id ExtendInteger();			/* Extend integer immediates. */
 extern int TySize();				/* Returns size in bytes of a type. */
 int opm = GetTxOpCodeX(pm);
 int opg = GetTxOpCodeX(pg);
						/* Opcodes */
 OperandType tym = GetTxOperandType( pm, indm );
 OperandType tya = GetTxOperandType(pm,2);
 OperandType tyb = GetTxOperandType(pm,3);
 OperandType tyc = GetTxOperandType(pg,indc);
 OperandType tycp;
						/* Operand types */
 int szm = TySize(tym);
 int sza = TySize(tya);
 int szb = TySize(tyb);
 int szc = TySize(tyc);
 int szcp;
						/* Operand sizes */
 boolean snc = IsSigned(tyc);
 boolean sncp;
						/* Operand signed flags */
 AN_Id adm = GetTxOperandAd(pm,indm);
 AN_Id adb = GetTxOperandAd(pm,3);
 AN_Id adc = GetTxOperandAd(pg,indc);
 AN_Id adcp;
						/* Operand addresses */

 *pty = tyc;					/* set default return values */
 *pad = adc;
 if(!IsOpGMove(opm))
	return(FALSE);
 if(!IsOpGeneric(opg))
	 return(FALSE);

						/* Check node types */

 if(IsFP(tyb) || IsFP(tyc))	/* Perform check for floating point operands. */
	{if((szb != szc) || (szb < sza))	/* Check that floating point */
		{return(FALSE);			/* operands get data bytes */
		}				/* and correct accuracy. */
#ifdef W32200
	 else if(IsFP(tyb) && IsAdIndexRegScaling(adb))	/* Indexed-Register- */
		{return(FALSE);				/* with-Scaling	*/
		}
	 else if(IsFP(tyc) && IsAdIndexRegScaling(adc))	/* illegal with	*/
		{return(FALSE);				/* floating-point. */
		}
#endif
	 else
		{*pad = adm;		/* report floating point types valid */
		 *pty = tym;
		 return(TRUE);
		}
	} /* END OF if(IsFP(tyb) || IsFp(tyc)) */

						/* Integer types if here. */
 tycp = ( sza < szc ) ? tya : tyc;		/* Compute integer trial type */
 szcp = TySize(tycp);				/* and address.	*/
 sncp = IsSigned(tycp);
 adcp = adm;
	
				/* Perform checks for integer operands */
 if((szm != szcp) && !IsAdTIQ(adm))	/* Check that can change type,	*/
	return(FALSE);			/* if necessary.	*/
 if((szb < szcp) && !IsAdCPUReg(adb))	/* Check that C is not picking up  */
	return(FALSE);			/* data bytes that are not in A */
 if((szb < szc) && !IsAdCPUReg(adb))		/* Check that not losing */
	return(FALSE);				/* bytes that C was	*/
						/* picking up.	*/
				/* If the type of C is truncating an */
				/* immediate, perform the truncation here. */
 if(IsAdImmediate(adm) && (szc < TySize(Tword)))
	{if(!IsAdNumber(adm)) return(FALSE);
	 adcp = ExtendInteger(tyc,adcp);
	}
/* Next check prevents the optimization in cases like
	MOVW	{uhalf}s,%r0
	CMPW 	&65536,%r0
	-->
	CMPW	&65536,{uhalf}%r0
which is likely to be followed by a conditional branch which
checks the C bit of the PSW (because we are doing unsigned
compares).  The optimized code is then wrong because the C bit
it set only if there is a borrow from bit 17.
For example, we have to be careful to avoid the following
optimization sequence:
	MOVH    2(%ap),%r8
	movzhw  %r8,%r0
	CMPW    &131072,%r0 --> CMPH	&131072,2(%ap)
	jleu			jleu
which is again incorrect.
The conservative approach is to disallow the optimization
unless the jump is je or jne.
*/

 if(IsUnSigned(tya) && opg == G_CMP && sza != szc ) {
 	int opnext = GetTxOpCodeX(GetTxNextNode(pg));
	if(opnext != IJE && opnext != IJNE)
		return(FALSE);
 }

 if((szc < 4) && (snc != sncp))	/* Check that C gets the correct extension */
	return(FALSE);
				/* Check that opcodes that execute differently
				on unsigned words get the correct type */
 if(((opg == G_MUL3) || (opg == G_DIV3) || (opg == G_MOD3)) &&
		(tyc  == Tuword) && (snc != sncp))
	return(FALSE);

#ifdef W32200
 if(szm != szcp)				/* If size of operand changes,*/
	{if(IsAdAutoAddress(adcp))		/* if auto-address or	*/
		return(FALSE);			/* indexed-	*/
	 if(IsAdIndexRegScaling(adcp))		/* register-with-scaling */
		return(FALSE);			/* FALSE.	*/
	}
#endif

 *pty = tycp;
 *pad = adcp;
 return(TRUE);
					/* Report integer types valid */
}
	boolean
IsInstrDeleteOrMoveAllowed(pf,pl)	/* TRUE if conditions that are */
					/* required for either delete or move */
					/* in peephole 22x are met.*/
TN_Id pf;		/* Text id of first of a pair of instructions. */
TN_Id pl;		/* Text id of second of a pair of instructions. */

{extern FN_Id FuncId;	/* Id of current function. */
 extern boolean IsDispAPFP(); /* TRUE is displavement from AP or FP. */
 AN_Id adf2;		/* Address id of operand 2 of first instruction. */
 AN_Id adf3;		/* Address id of operand 3 of first instruction. */
 extern enum CC_Mode ccmode;
 unsigned int opl;	/* Opcode of second instuction. */
 unsigned int regno;	/* Register number. */

 adf2 = GetTxOperandAd(pf,2);
 if(!IsAdSafe(adf2) || IsTxOperandVol(pf,2))	/* last src of OP1 not volatile */
	return(FALSE);
 adf3 = GetTxOperandAd(pf,3);
 if(!IsAdSafe(adf3) || IsTxOperandVol(pf,3))	/* dest of OP1 not volatile */
	return(FALSE);
 if(IsAdCPUReg(adf3) && (regno = GetRegNo(GetAdRegA(adf3))) >= GetRegNo(CREG9)
		&& regno <= GetRegNo(CREG15)) 
					/* B is not special register */
	return(FALSE);
 if(IsTxCBr(pl))			/* Not allowed if last instruction is */
	return(FALSE);			/* a conditional jump.	*/
 if(IsTxCBr(pf))			/* Not allowed if first	*/
	return(FALSE);			/* instruction is a conditional jump. */
 opl = GetTxOpCodeX(pl);
 if(IsOpUses(opl,adf3))			/* dst of OP1 not used by OP2 */
	return(FALSE);
 if(ccmode == Transition && IsFnSetjmp(FuncId) && (opl == ICALL || opl == CALL )
		&& IsDispAPFP(adf3))
					/* If setjmp in file, don't move */
					/* stack variable set passed a call. */
	return(FALSE);
 return(TRUE);				/* All conditions are met. */
}
	boolean
IsInstrDeleteAllowed(pf,pl)		/* TRUE if conditions unique to */
					/* deleting in peephole 22x are met. */
TN_Id pf;		/* Text id of first of a pair of instructions. */
TN_Id pl;		/* Text id of second of a pair of instructions. */

{
 extern boolean IsOpSets(); /* TRUE if opcode sets address. */
 AN_Id adf3;		/* Address id of operand 3 of first instruction. */
 AN_Id adl3;		/* Address id of operand 3 of second instruction. */
 unsigned int opl;	/* Opcode of second instruction. */
 int szf3;		/* Size in bytes of operand 3 of first instruction. */
 int szl3;		/* Size in bytes of operand 3 of second instruction. */


 adf3 = GetTxOperandAd(pf,3);
 adl3 = GetTxOperandAd(pl,3);
 szf3 = TySize(GetTxOperandType(pf,3));
 szl3 = TySize(GetTxOperandType(pl,3));
 opl = GetTxOpCodeX(pl);

 if(!(
 	(adf3 == adl3 && (szf3 <= szl3 || IsAdCPUReg(adl3)))
	|| IsOpSets(opl,adf3) 
	|| IsDeadAd(adf3,pl)
     )
   )
					/* dst of OP1 is set by OP2 */
					/* or is dead after OP2 */
		return(FALSE);
 if(setslivecc(pf))			 /* its condition codes are not needed*/
	return(FALSE);
 return(TRUE);
}
	boolean
IsInstrMoveAllowed(pf,pl,peepno)	/* TRUE is pf can be moved passed pl. */
TN_Id pf;		/* Text id of first of a pair of instructions. */
TN_Id pl;		/* Text id of second of a pair of instructions. */
int peepno;		/* Number of peephole optimization. */

{extern boolean IsOpSets(); /* TRUE if opcode sets address. */
 extern boolean IsTxSets(); /* TRUE if text node sets address. */
 extern enum CC_Mode ccmode; /* -Xt or -Xa or -Xc?	*/
 AN_Id adf2;		/* Address id of operand 2 of first instruction. */
 AN_Id adf3;		/* Address id of operand 3 of first instruction. */
 AN_Id adl1;		/* Address id of operand 1 of second instruction. */
 AN_Id adl2;		/* Address id of operand 2 of second instruction. */
 AN_Id adl3;		/* Address id of operand 3 of second instruction. */
 unsigned int opl;	/* Opcode of second instuction. */
 int szl2;		/* Size of operand 2 of second instruction. */
 int szl3;		/* Size of operand 3 of second instruction. */
 OperandType tyl1;	/* Operand type of operand 1 of second instruction. */
 OperandType tyl2;	/* Operand type of operand 2 of second instruction. */
 OperandType tyl3;	/* Operand type of operand 3 of second instruction. */

#ifdef W32200
 extern AN_Id SPPostIncr;	/* Address of (%sp)+ */
 unsigned int regnol1;	/* Register number for operand 1 of 2nd instruction. */
 unsigned int regnol2;	/* Register number for operand 2 of 2nd instruction. */
#endif

					/* cannot move an aliasable quantity */
					/* passed a call */
 opl = GetTxOpCodeX(pl);
 adf2 = GetTxOperandAd(pf,2);
 adf3 = GetTxOperandAd(pf,3);
 if((opl == CALL || opl == ICALL) 
		&& (	!IsAdNAQ(adf2)
		     || !IsAdNAQ(adf3)
		   )
   )
	return(FALSE);
 					/* anti-looping constraints */
					/* These constraints must superset */
					/* cases allowed by the peephole */
					/* optimizations that allow moving */
					/* instructions. */
					/* The peephole number is used to */
					/* establish a priority among the */
					/* optimizations that move code. */
					/* Lower numbers are higher priority. */
 opl = GetTxOpCodeX(pl);
 tyl1 = GetTxOperandType(pl,1);
 tyl2 = GetTxOperandType(pl,2);
 tyl3 = GetTxOperandType(pl,3);
 szl2 = TySize(tyl2);
 szl3 = TySize(tyl3);
 adl1 = GetTxOperandAd(pl,1);
 adl2 = GetTxOperandAd(pl,2);
 adl3 = GetTxOperandAd(pl,3);
					/* peepholes 220,221 and 222 */
					/* - const src */
 if(peepno >= 220
	&& (opl == G_MOV || opl == G_ADD3 || opl == G_SUB3)
 	&& IsAdImmediate(adl1) && IsAdNumber(adl1)
#ifdef W32200
	&& !IsAdAutoAddress(adl3)
#endif
 	&& (tyl1 == Tword || tyl1 == Tuword)
	&& (tyl2 == Tword || tyl2 == Tuword)
 	&& (tyl3 == Tword || tyl3 == Tuword)
	&& IsAdSafe(adl2) && !IsTxOperandVol(pl,2)
 	&& IsAdSafe(adl3) && !IsTxOperandVol(pl,3)
   )
	{if(peepno > 222)
		return(FALSE);
	 if(ccmode == Transition)
		{if(!IsAdAddrIndex(adf3)
			|| !IsAdAddrIndex(adl3)
			|| (GetAdAddrIndex(adf3) >= GetAdAddrIndex(adl3))
		   )
			return(FALSE);
		}
	 else	/*
		 * the situation in the ANSI modes is that addresses are
		 * usually nonvolatile and the approach is slightly different.
		 * we observe that in order to flip two instructions, their
		 * destinations cannot alias each other. so either one is a
		 * NAQ and the other isn't, or both are NAQs but different.
		 * the 2 NAQ case is ordered by their NAQness. the 1 NAQ case
		 * stops on the first destination being nonNAQ. this however
		 * unnecessarily kills off some PEEP#222s, so an additional
		 * allowance is made if the second dest uses the first. this
		 * allowance is o.k. since in all cases handled by 220-222, 
		 * the post-flip condition is that the second dest cannot use
		 * the first and in fact not candidates for flipping again.
		 */
		{if(!IsAdUses(adl3,adf3)
			&& !IsAdAddrIndex(adf3)
			|| IsAdAddrIndex(adl3)
			&& (GetAdAddrIndex(adf3) >= GetAdAddrIndex(adl3))
		   )
			return(FALSE);
		}
	}

#ifdef W32200
					/* peephole 223 - reg sources from  */
					/* different sets */
 if(peepno >= 223
	&& cpu_chip == we32200 
	&& opl == G_ADD3
	&& IsAdCPUReg(adl1)
	&& IsAdCPUReg(adl2)
	&& !IsAdAutoAddress(adl3)
 	&& (tyl1 == Tword || tyl1 == Tuword)
 	&& (tyl2 == Tword || tyl2 == Tuword)
	&& (tyl3 == Tword || tyl3 == Tuword)
 	&& IsAdSafe(adl3) && !IsTxOperandVol(pl,3)
	&& (peepno > 223 || !IsAdAddrIndex(adf3) || !IsAdAddrIndex(adl3) 
		|| (GetAdAddrIndex(adf3) >= GetAdAddrIndex(adl3)))
    )
	{regnol1 = GetRegNo(GetAdRegA(adl1));
	 regnol2 = GetRegNo(GetAdRegA(adl2));
	 if((regnol1 < 16 || regnol2 < 16) && (regnol1 > 15 || regnol2 > 15))
		return(FALSE);
	}
					/* peephole 224 - sources same reg */
					/* from 1st set */
 if(peepno >= 224
	&& cpu_chip == we32200
	&& opl == G_ADD3
	&& IsAdCPUReg(adl1)
	&& adl2 == adl1
	&& IsAdCPUReg(adl3)
 	&& (tyl1 == Tword || tyl1 == Tuword)
 	&& (tyl2 == Tword || tyl2 == Tuword)
	&& (tyl3 == Tword || tyl3 == Tuword)
	&& (peepno > 224 || !IsAdAddrIndex(adf3) || !IsAdAddrIndex(adl3) 
		|| (GetAdAddrIndex(adf3) >= GetAdAddrIndex(adl3)))
    )
	{regnol2 = GetRegNo(GetAdRegA(adl2));
	 if(regnol2 < 16)
		return(FALSE);
	}
					/* peephole 225 - shift by 2 */
 if(peepno >= 225
	&& cpu_chip == we32200
	&& (opl == G_ALS3 || opl == G_LLS3)
 	&& IsAdImmediate(adl1) && IsAdNumber(adl1) 
			&& GetAdNumber(tyl1,adl1) == 2
	&& IsAdCPUReg(adl2)
	&& IsAdCPUReg(adl3)
 	&& (tyl1 == Tword || tyl1 == Tuword)
 	&& (tyl2 == Tword || tyl2 == Tuword)
	&& (tyl3 == Tword || tyl3 == Tuword)
	&& (peepno > 225 || !IsAdAddrIndex(adf3) || !IsAdAddrIndex(adl3) 
		|| (GetAdAddrIndex(adf3) >= GetAdAddrIndex(adl3)))
    )
	{regnol2 = GetRegNo(GetAdRegA(adl2));
	 if(regnol2 < 16)
		return(FALSE);
	}
#endif

					/* peephole 227 - */
					/* - merging type conversions */
 if(peepno >= 227
	&& IsOpGMove(opl)
#ifdef W32200
	&& !IsAdAutoAddress(adl3)
#endif
	&& (szl2 < szl3)
 	&& IsAdSafe(adl2) && !IsTxOperandVol(pl,2)
 	&& IsAdSafe(adl3) && !IsTxOperandVol(pl,3)
	&& (peepno > 227 || !IsAdAddrIndex(adf3) || !IsAdAddrIndex(adl3) 
		|| (GetAdAddrIndex(adf3) >= GetAdAddrIndex(adl3)))
   )
	return(FALSE);
					/* not de-optimizing transformation */
 if(!IsAdCPUReg(adf2) && IsAdCPUReg(adf3)) 
	return(FALSE);
 if(!IsAdMAUReg(adf2) && IsAdMAUReg(adf3)) 
	return(FALSE);
					/* dst of OP2 would not be */
					/* overwritten by OP1 */
 if(adl3 == adf3 
	    || (	!IsAdNAQ(adl3) 
		   && 	!IsAdNAQ(adf3) 
		   && 	!IsAdSNAQ(adl3) 
		   && 	!IsAdSNAQ(adf3)
	       ) 
	    || IsTxSets(pf,adl3)
   )
	return(FALSE);
 if(IsTxBr(pl))				/* don't move past branch */
	return(FALSE);

#ifdef W32200
					/* operand of OP1 would not be */
					/* overwritten by OP2 */
					/* This test only covers setting */
					/* by the opcode or the destination. */
					/* Setting by auto addressing is */
					/* taken care of in the operand */
					/* checks. */
 if(SPPostIncr == (AN_Id) NULL)		/* If we don't know address of (%sp)+ */
	{if(cpu_chip == we32200)
		SPPostIncr = GetAdPostIncr(Tword,"",CSP);
	}
#endif

 if(	(adf2 == adl3)
    ||	(	!IsAdNAQ(adf2)
	    &&	!IsAdNAQ(adl3)
	    &&	!IsAdSNAQ(adf2)
	    &&	!IsAdSNAQ(adl3)
#ifdef W32200
	    &&	(adl3 != SPPostIncr)
#endif
	)
    ||	IsOpSets(opl,adf2)
   )
	return(FALSE);
 if(setslivecc(pf))		/* OP1 does not provide cond codes */
	return(FALSE);
 if(setslivecc(pl))		/* OP2 does not provide cond codes */
	return(FALSE);
 return(TRUE);			/* Conditions satisfied. */
}
	AN_Id
ExtendInteger(type,an_id)		/* Return the AN_Id for the immediate */
					/* constant with the zero or sign */
					/* extension appropriate for the */
					/* given type. */ 
OperandType type;			/* Operand type for extension . */
AN_Id an_id;				/* AN_Id of immediate to be extended. */
{extern long int ExtendItem();	/* Adjusts item to comply with its type. */
 extern void fatal();			/* Handles fatal errors. */
 extern AN_Id i0;			/* AN_Id of immediate zero. */
 long int num;				/* Numeric value of immediate. */

 switch(type)				/* Consider various types. */
 	{
	 case Tsbyte:
	 case Tbyte:
	 case Tubyte:
	 case Tshalf:
	 case Thalf:
	 case Tuhalf:
 		if(!IsAdImmediate(an_id) /* Numeric integer immed? */
				|| !IsAdNumber(an_id)) 
			fatal("ExtendInteger: Not a numeric immediate.\n");
 		num = GetAdNumber(Tword,an_id);	/* Get value of immediate. */
		num = ExtendItem(type,num);	/* Resize as required.	*/
		an_id = GetAdAddToKey(Tword,i0,num); /*Get extended immediate.*/
		return(an_id);
	 case Tword:
	 case Tsword:
	 case Tuword:
		return(an_id);		/* No extension necessary. */
	 default:
		fatal("ExtendInteger: illegal type (%d).\n",type);
		/*NOTREACHED*/
	}
}
	boolean
IsDeadAd(an_id,t)			/* TRUE if an_id is dead following t */
AN_Id an_id;				/* Address Id */
TN_Id t;				/* Text node pointer */

{unsigned int addrind;			/* address index */
 extern unsigned ld_maxbit;

 if(!IsAdAddrIndex(an_id))		/* If node is not in GNAQ list, */
	return(FALSE);			/* it cannot have l/d analysis */	
		
 addrind = GetAdAddrIndex(an_id);	/* get GNAQ index */
					/* If does not fit in live vector, */
					/* cannot have l/d analysis */
 if(addrind >= ld_maxbit)
	return(FALSE);

					/* Is the bit clear in the live */
					/* vector? */
 if(((t->nlive[addrind/WDSIZE] >> (addrind%WDSIZE)) & 0x1) != 0)
	return(FALSE);
 return(TRUE);
}
	boolean
islivecc(tn_id)			/* TRUE if live condition codes. */
TN_Id tn_id;

{extern AN_Id A;		/* AN_Id of A Condition Code.	*/
 extern AN_Id C;		/* AN_Id of C Condition Code.	*/
 extern AN_Id N;		/* AN_Id of N Condition Code.	*/
 extern AN_Id V;		/* AN_Id of V Condition Code.	*/
 extern AN_Id Z;		/* AN_Id of Z Condition Code.	*/
#ifdef W32200
 extern AN_Id X;		/* AN_Id of X Condition Code.	*/
 extern m32_target_cpu cpu_chip;		/* Type of CPU chip.	*/
#endif
 extern m32_target_math math_chip;		/* Type of MATH chip.	*/
 register unsigned int mask;	/* Mask for condition code bits.	*/
 unsigned long int Live;	/* Space for Live bits.	*/

	/* N.B.: HERE WE ASSUME THAT ALL THE CONDITION CODES WILL GO */
	/*	INTO THE SAME (THE FIRST) WORD.	*/

 mask = 0;
 if((math_chip == we32106) || (math_chip == we32206))
	if(IsAdAddrIndex(A))
		mask |= (1 << GetAdAddrIndex(A));
 if(IsAdAddrIndex(C))
 	mask |= (1 << GetAdAddrIndex(C));
 if(IsAdAddrIndex(N))
 	mask |= (1 << GetAdAddrIndex(N));
 if(IsAdAddrIndex(V))
 	mask |= (1 << GetAdAddrIndex(V));

#ifdef W32200
 if(cpu_chip == we32200)
	if(IsAdAddrIndex(X))
	 	mask |= (1 << GetAdAddrIndex(X));
#endif

 if(IsAdAddrIndex(Z))
 	mask |= (1 << GetAdAddrIndex(Z));

 GetTxLive(tn_id,&Live,1);			/*Get first word of live bits.*/
 mask &= Live;					/* Live condition codes. */
 if(mask == 0)					/*  */
	return(FALSE);
 return(TRUE);
}
	boolean
IsDispAPFP(an_id)		/* TRUE if address is displacement from AP|FP.*/
AN_Id an_id;			/* AN_Id of address.	*/

{register RegId regid;		/* Register identifier.	*/

 if (IsAdDisp(an_id) &&
     ((regid = GetAdRegA(an_id)) == CFP || regid == CAP))
	return(TRUE);
 else
	return(FALSE);
}

	/*ARGSUSED*/
	long int
AddressDelta(type,an_id)	/* Returns the amount that an address */
				/* changes its first register in bytes. */
				/* Returns zero for all but auto address */
				/* modes. */
OperandType type;		/* Type of the operand. */
AN_Id an_id;			/* Address ID of operand. */

{extern void fatal();		/* Handles fatal errors.	*/

 switch(GetAdMode(an_id))
	{
	 case PreDecr:
	 case PostDecr:
		return(-TySize(type));
	 case PreIncr:
	 case PostIncr:
		return(TySize(type));
	 case Immediate:
	 case Absolute:
	 case AbsDef:
	 case CPUReg:
	 case Disp:
	 case DispDef:
	 case IndexRegDisp:
	 case IndexRegScaling:
	 case MAUReg:
	 case StatCont:
	 case Raw:
		endcase;
	 default:
		fatal("AddressDelta: unknown mode.\n");
		endcase;
	} /* END OF switch(GetAdMode(an_id)) */
 return(0);
}
	long int
EffectiveDisp(type,an_id)	/* Returns the effective address of */
				/* displacement operands with numeric */
				/* displacement.*/
OperandType type;		/* Type of operand. */
AN_Id an_id;			/* Address Id of operand. */
{extern void fatal();		/* Handles fatal errors.	*/

 if(!IsAdNumber(an_id))				/* This is for numeric	*/
						/* displacements only.	*/
	fatal("EffectiveDisp: non-numeric displacement presented.\n");

 switch(GetAdMode(an_id))	/* Compute effective displacement. */
	{
#ifdef W32200
	 case PreIncr:
		return(TySize(type));
	 case PreDecr:
		return(-TySize(type));
	 case IndexRegDisp:			/* These have displacements, */
	 case IndexRegScaling:			/* but we cannot compute */
#endif
	 case Disp:
		return(GetAdNumber(type,an_id));
		/*NOTREACHED*/
		endcase;
	 case Immediate:
	 case Absolute:
	 case AbsDef:
	 case CPUReg:
	 case DispDef:
#ifdef W32200
	 case PostDecr:
	 case PostIncr:
#endif
	 case MAUReg:
	 case StatCont:
	 case Raw:
		return(0);
		/*NOTREACHED*/
		endcase;
	 case Undefined:
	 default:
		fatal("EffectiveDisp: unknown mode\n");
		endcase;
	} /* END OF switch(GetAdMode(an_id) */
 /*NOTREACHED*/
}
	boolean
IsSameEffAdd(FirstI,FirstO,LastI,LastO)
TN_Id FirstI;			/* TN_Id of instruction containing the	*/
				/* first address.	*/
unsigned int FirstO;		/* Operand number of first address.	*/
TN_Id LastI;			/* TN_Id of instruction containing the	*/
				/* last address.	*/
unsigned int LastO;		/* Operand number of last address.	*/

{extern int SameEffAdd();

 if(SameEffAdd(FirstI,FirstO,LastI,LastO) == 1)
	return(TRUE);
 return(FALSE);
}

#ifdef LINT
	boolean
IsDiffEffAdd(FirstI,FirstO,LastI,LastO)
TN_Id FirstI;			/* TN_Id of instruction containing the	*/
				/* first address.	*/
unsigned int FirstO;		/* Operand number of first address.	*/
TN_Id LastI;			/* TN_Id of instruction containing the	*/
				/* last address.	*/
unsigned int LastO;		/* Operand number of last address.	*/

{extern int SameEffAdd();

 if(SameEffAdd(FirstI,FirstO,LastI,LastO) == -1)
	return(TRUE);
 return(FALSE);
}
#endif
	int
SameEffAdd(FirstI,FirstO,LastI,LastO)
TN_Id FirstI;			/* TN_Id of instruction containing the	*/
				/* first address.	*/
unsigned int FirstO;		/* Operand number of first address.	*/
TN_Id LastI;			/* TN_Id of instruction containing the	*/
				/* last address.	*/
unsigned int LastO;		/* Operand number of last address.	*/

{AN_Id FirstA;			/* Name of first operand.	*/
 long int FirstADelta;		/* Changes to first address A register.	*/
 long int FirstDisp;		/* Displacement of first address.	*/
 AN_Mode FirstM;		/* Mode of first operand.	*/
 OperandType FirstT;		/* Type of first operand.	*/
 RegId FirstRA;			/* Register A of first operand.	*/
 AN_Id LastA;			/* Name of last operand.	*/
 long int LastDisp;		/* Displacement of last address.	*/
 AN_Mode LastM;			/* Mode of last operand.	*/
 RegId LastRA;			/* Register A of last operand.	*/
 OperandType LastT;		/* Type of last operand.	*/
 extern unsigned int Max_Ops;	/* Maximum operands in an instruction.	*/
 AN_Id an_id;			/* Identifier of an address node.	*/
 extern void fatal();			/* Handles fatal errors. */
 unsigned int operand;
 RegId regid;			/* Identifier of a register.	*/
 TN_Id tn_id;			/* Identifier of a text node.	*/
 OperandType type;		/* Type of an operand.	*/

#ifdef W32200
 long int FirstBDelta;		/* Changes to first address B register.	*/
 RegId FirstRB;			/* Register B of first oeprand.	*/
 RegId LastRB;			/* Register B of last operand.	*/
#endif

 if((FirstI == LastI) && (FirstO == LastO))	/* Identity case?	*/
	return(1);
 if(IsTxLabel(FirstI))				/* First and last should */
	fatal("SameEffAdd: first is label.\n");
 if(IsTxLabel(LastI))				/* not be labels.	*/
	fatal("SameEffAdd: last is label.\n");
 FirstA = GetTxOperandAd(FirstI,FirstO);	/* Get address data.	*/
 FirstM = GetAdMode(FirstA);
 FirstT = GetTxOperandType(FirstI,FirstO);
 LastA = GetTxOperandAd(LastI,LastO);
 LastM = GetAdMode(LastA);
 LastT = GetTxOperandType(LastI,LastO);

#ifdef W32200
 FirstRB = REG_NONE;
#endif

 switch(FirstM)
	{case AbsDef:
	 case DispDef:
		if(	(FirstI != LastI)
		    &&	(	(FirstA == GetTxOperandAd(FirstI,DESTINATION))
			    ||	(GetTxNextNode(FirstI) != LastI)
			)
		  )
			return(0);		/* Can't tell.	*/
		/* FALLTHRU */
	 case Immediate:			/* These are the easy cases. */
	 case Absolute:
	 case CPUReg:
	 case MAUReg:
	 case StatCont:
	 case Raw:
		if(FirstA == LastA)
			return(1);
		else
			return(-1);
		/*NOTREACHED*/
		endcase;
#ifdef W32200
	 case IndexRegDisp:		     /*These are the most difficult.*/
	 case IndexRegScaling:
		FirstRB = GetAdRegB(FirstA);
		/* FALL THROUGH INTENTIONALLY.	*/
	 case PreDecr:			     /*These are in between.*/
	 case PostDecr:
	 case PostIncr:
#endif
	 case Disp:
		if(!IsAdNumber(FirstA))		/* Is displacement numeric? */
			return(0);		/* No: we don't know.	*/
		FirstDisp = EffectiveDisp(FirstT,FirstA);
		FirstRA = GetAdRegA(FirstA);
		endcase;
	 default:
		fatal("SameEffAdd: unknown mode (0x%x).\n",FirstM);
		endcase;
	} /* END OF switch(FirstM) */

					/* Scan all intermediate operands */
					/* and correct for any diddling	*/
					/* on the way by.	*/
 FirstADelta = 0;			/* Changes so far to registers. */
 operand = FirstO;
 tn_id = FirstI;

#ifdef W32200
 FirstBDelta = 0;
#endif

 while((tn_id != LastI) || (operand != LastO))
	{an_id = GetTxOperandAd(tn_id,operand);

#ifdef W32200
	 if(IsAdAutoAddress(an_id))
		regid = GetAdRegA(an_id);
	 else
		regid = REG_NONE;
#else
	regid = REG_NONE;
#endif

	 type = GetTxOperandType(tn_id,operand);
	 if(regid == FirstRA)
		FirstADelta += AddressDelta(type,an_id);

#ifdef W32200
	 if((regid == FirstRB) && (regid != REG_NONE))
		FirstBDelta += AddressDelta(type,an_id);
#endif

	 if(++operand == Max_Ops)
		{operand = 0;
		 tn_id = GetTxNextNode(tn_id);
		 if(IsTxLabel(tn_id))		/* If a label, all bets	*/
			return(0);		/* are off.	*/
		}
	} /* END OF while((tn_id != LastI && (operand != LastO)) */

 switch(LastM)
	{case Immediate:			/* These are the easy cases. */
	 case Absolute:
	 case AbsDef:
	 case CPUReg:
	 case DispDef:
	 case MAUReg:
	 case StatCont:
	 case Raw:
		return(-1);
#ifdef W32200
	 case IndexRegDisp:		     /*These are the most difficult.*/
	 case IndexRegScaling:
		LastRB = GetAdRegB(LastA);
		if(LastRB != FirstRB)
			return(0);		/* We don't know.	*/
		if(FirstBDelta != 0)
			return(0);		/* We don't know.	*/
		/*FALL THROUGH INTENTIONALLY.	*/
	 case PreDecr:				/* These are in between. */
	 case PreIncr:
	 case PostDecr:
	 case PostIncr:
#endif
	 case Disp:	
		if(!IsAdNumber(LastA))
			return(0);		/* We can't tell.	*/
		LastDisp = EffectiveDisp(LastT,LastA);
		LastRA = GetAdRegA(LastA);
		if(LastRA != FirstRA)
			return(-1);
		if(FirstDisp != (LastDisp + FirstADelta))
			return(-1);
		else
			return(1);
		/*NOTREACHED*/
		endcase;
	 default:
		fatal("SameEffAdd: unknown mode (0x%x).\n",LastM);
		endcase;
	} /* END OF switch(LastM) */

 fatal("SameEffAdd: logic failure.\n");
 /*NOTREACHED*/
}
	long int
ExtendItem(type,num)		/* Make item conform to its data type.	*/
OperandType type;		/* Required type of item.	*/
long int num;			/* Value of item.	*/

{unsigned int TySizeT;		/* TySize(type).	*/
 unsigned int TySizeW;		/* TySize(Tword).	*/
 int bit_size;			/* Size of the data type, in bits.	*/
 extern void fatal();		/* Handles fatal errors.	*/

 TySizeW = TySize(Tword);			/* Validate type.	*/
 if((TySizeT = TySize(type)) == TySizeW)
	return(num);
 if(TySizeT > TySizeW)
	fatal("ExtendItem: TySizeT (%u) too big.\n",TySizeT);

 bit_size = TySizeT * B_P_BYTE;		/* Get data size in bits. */
					/* The following statement creates a */
					/* mask for extracting bit_size low */
					/* order bits as follows(using Tsbyte */
					/* as an example): */
					/*   (a) shift a '1' left 8 bits */
					/* 	    ...000100000000	 */
					/*   (b) complement it           */
					/*	    ...111011111111	 */
					/*   (c) increment it		 */
					/*	    ...111100000000	 */
					/*   (d) complement again	 */
					/*	    ...000011111111	 */
					/* Voila! a mask!		 */
 num &= ~((~(1 << bit_size)) + 1);	/* Mask off upper bits. */
 if(IsSigned(type))			/* Extend sign, if appropriate. */
					/* The following statement extracts */
					/* and extends the sign as follows */
					/* (using Tsbyte as an example):   */
					/*   (a) shift a '1' left 7 bits */
					/*	    ...000010000000	 */
					/*   (b) extract the sign	 */
					/* 	    ...0000s0000000	 */
					/*   (c) complement it (c = ~s)	 */
					/*          ...1111c1111111	 */
					/*   (d) increment it		 */
					/*          ...sssss0000000	 */
					/* This is then or'd into the 	 */
					/* number to extend the sign.    */
	num |= 	(~(num & (1 << (bit_size - 1)))) + 1;
 return(num);
}
