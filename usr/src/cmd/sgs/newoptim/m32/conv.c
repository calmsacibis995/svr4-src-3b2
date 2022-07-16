/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)newoptim:m32/conv.c	1.12"

/* Routines in this file do the input/output instruction conversions */

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include "defs.h"
#include "debug.h"
#include "optab.h"
#include "OpTabTypes.h"
#include "OpTabDefs.h"
#include "RegId.h"
#include "RegIdDefs.h"
#include "OperndType.h"
#include "RoundModes.h"
#include "ANodeTypes.h"
#include "ANodeDefs.h"
#include "olddefs.h"
#include "TNodeTypes.h"
#include "optim.h"
#include "optutil.h"
#include "LoopTypes.h"
#include "TNodeDefs.h"
#include "ALNodeType.h"
#include "FNodeTypes.h"
#include "FNodeDefs.h"

	/* private function declarations */
STATIC void Bconv();	/* converts cpu branches into is25 jmps.	*/
STATIC void genin();	/* converts input to generics.	*/
STATIC void genout();	/* converts generics for output. 	*/
STATIC void i25in();	/* converts is25 instr to generics.	*/
STATIC void i25out();	/* converts is25 instr for output.	*/
STATIC boolean togen();	/* map to internal generics.	*/
STATIC void frgen();	/* map from generics to cpu.	*/
STATIC TN_Id cputomis();/* converts #TYPEs to mis moves.*/
STATIC TN_Id mistocpu();/* undoes cputomis().		*/
	void
inconv()		/* main routine for input conversions 
			 * the possible input instructions are:
			 * 1) IS25 instructions - all jmps (including rets)
			 * 	are untouched. as much of the rest are converted
			 *	into an internal generic form.
			 * 2) CPU instructions - all pc-relative branches are
			 *	converted into IS25 counterparts. this is done so
			 *	that PCI (reord()) is free to move basic blocks
			 *	around. the rest are converted into
			 *	the internal generic form.
			 * 3) MIS instructions - except for branches, are converted
			 *	into the generic form.
			 * 4) Aux Nodes - mostly ignored, except for 'TYPE'.
			 * 5) Pseudo Nodes - (.align, .ln etc.) ignored.
 			 */
{
 extern FN_Id FuncId;
 void funcprint();	/* Prints function.	*/
 register unsigned short o;
 register TN_Id p;

 if(DBdebug(0,XINCONV))			/* Print function before */
	funcprint(stdout,"inconv: before mapping to internal generics",0);

 		/* Pass 1 - convert as much as possible to internal generics */
 for(ALLN(p))
 {
	o = GetTxOpCodeX(p);
	if(IsOpIs25(o))
		i25in(p);
	else if(IsOpAux(o)){
		if(o == TYPE)
			PutFnMISconvert(FuncId,TRUE);
		}
	else if(IsOpPseudo(o))
		continue;		/* don't go near these */
	else 
		genin(p);
 }

 		/* Pass 2 - collapse #TYPE nodes */
 if(IsFnMISconvert(FuncId))
	for(ALLN(p))
	{
		o = GetTxOpCodeX(p);
		if(IsOpAux(o)){
			switch(o){
			case TYPE:
				p = cputomis(p);
				endcase;
			}
		}
	}

 if(DBdebug(0,XINCONV))			/* Print function after */
	funcprint(stdout,"inconv: after mapping to internal generics",0);
}
	void
outconv()		/* main routine for output conversions.
			 * all generics are converted into output form,
			 * driven by optab[].
			 * of what remains, only 'save' and 'ret' are
			 * converted into their CPU counterparts.
			 */
{
 extern FN_Id FuncId;
 void funcprint();	/* Prints function.	*/
 register unsigned short o;
 register TN_Id p;

 if(DBdebug(0,XOUTCONV))			/* Print function before */
	funcprint(stdout,"outconv: before mapping to cpu instructions",0);

			/* pass 1 - convert MIS moves and pushes back into
			 *  CPU moves and pushes.
			 */
 if(IsFnMISconvert(FuncId)){
	for(ALLN(p))
	{
		o = GetTxOpCodeX(p);
		if(IsOpGeneric(o))
			switch(o){
			case G_MMOV:
				p = mistocpu(p);
				endcase;
			}
	}
 }

			/* pass 2 - convert generics and is25 (save/ret) to cpu */
 for(ALLN(p))
 {
	o = GetTxOpCodeX(p);
	if(IsOpGeneric(o))
		genout(p);
	else if(IsOpIs25(o))
		i25out(p);
 }

 if(DBdebug(0,XOUTCONV))			/* Print function after */
	funcprint(stdout,"outconv: after mapping to cpu instructions",0);
}
	STATIC void
genin(p)			/* Map from nongeneric instructions, i.e.
				 * cpu instructions, to internal generics.
				 */
register TN_Id p;
{
 extern AN_Id i1;		/* AN_Id of immediate 1.	*/
 extern AN_Id i0;		/* AN_Id of immediate 0.	*/
 unsigned short o;
 extern void prafter();
 extern void prbefore();

 if(IsTxBr(p)){			/* Convert CPU branches. */
	Bconv(p);
	return;			/* No generic forms for these. */
	}

 if(!togen(p))			/* Map to generics. */
	return;

				/* Now normalize into internal form. */
 switch(GetTxOpCodeX(p))
	{case G_ADD2:	o = G_ADD3; goto mapdy;
	 case G_AND2:	o = G_AND3; goto mapdy;
	 case G_CLR:	goto mapclr;
	 case G_DEC:	o = G_SUB3; goto mapdec;
	 case G_DIV2:	o = G_DIV3; goto mapdy;
	 case G_INC:	o = G_ADD3; goto mapinc;
	 case G_MOD2:	o = G_MOD3; goto mapdy;
	 case G_MUL2:	o = G_MUL3; goto mapdy;
	 case G_OR2:	o = G_OR3; goto mapdy;
	 case G_SUB2:	o = G_SUB3; goto mapdy;
	 case G_TST:	goto maptst;
	 case G_XOR2:	o = G_XOR3; goto mapdy;
	 case G_MFABS1:	o = G_MFABS2; goto mapmon;
	 case G_MFADD2:	o = G_MFADD3; goto mapdy;
	 case G_MFDIV2:	o = G_MFDIV3; goto mapdy;
	 case G_MFMUL2:	o = G_MFMUL3; goto mapdy;
	 case G_MFNEG1:	o = G_MFNEG2; goto mapmon;
	 case G_MFREM2:	o = G_MFREM3; goto mapdy;
	 case G_MFRND1:	o = G_MFRND2; goto mapmon;
	 case G_MFSQR1:	o = G_MFSQR2; goto mapmon;
	 case G_MFSUB2:	o = G_MFSUB3; goto mapdy;
	 default:
		return;
	} /* END OF switch(GetTxOpCodeX(p)) */

 mapdy:
	if(DBdebug(2,XINCONV))
		prbefore(GetTxPrevNode(p),GetTxNextNode(p),0,
				"genin: map to triadic");
	PutTxOpCodeX(p,o);
	PutTxOperandAd(p,1,GetTxOperandAd(p,2));
	PutTxOperandType(p,1,GetTxOperandType(p,2));
	PutTxOperandFlags(p,1,GetTxOperandFlags(p,2));
	PutTxOperandAd(p,2,GetTxOperandAd(p,3));
	PutTxOperandType(p,2,GetTxOperandType(p,3));
	PutTxOperandFlags(p,2,GetTxOperandFlags(p,3));
	if(DBdebug(2,XINCONV))
		prafter(GetTxPrevNode(p),GetTxNextNode(p),0);
	return;
 mapmon:
	if(DBdebug(2,XINCONV))
		prbefore(GetTxPrevNode(p),GetTxNextNode(p),0,
				"genin: map to dyadic");
	PutTxOpCodeX(p,o);
	PutTxOperandAd(p,2,GetTxOperandAd(p,3));
	PutTxOperandType(p,2,GetTxOperandType(p,3));
	PutTxOperandFlags(p,2,GetTxOperandFlags(p,3));
	if(DBdebug(2,XINCONV))
		prafter(GetTxPrevNode(p),GetTxNextNode(p),0);
	return;
	
 mapclr:
	if(DBdebug(2,XINCONV))
		prbefore(GetTxPrevNode(p),GetTxNextNode(p),0,
				"genin: map to mov");
	PutTxOpCodeX(p,G_MOV);
	PutTxOperandAd(p,2,i0);
	PutTxOperandType(p,2,Tword);
	PutTxOperandFlags(p,2,0);	
	if(DBdebug(2,XINCONV))
		prafter(GetTxPrevNode(p),GetTxNextNode(p),0);
	return;
	
 mapdec:
 mapinc:
	if(DBdebug(2,XINCONV))
		prbefore(GetTxPrevNode(p),GetTxNextNode(p),0,
				"genin: map to add/sub");
	PutTxOpCodeX(p,o);
	PutTxOperandAd(p,1,i1);
	PutTxOperandType(p,1,Tword);
	PutTxOperandFlags(p,1,0);	
	PutTxOperandAd(p,2,GetTxOperandAd(p,3));
	PutTxOperandType(p,2,GetTxOperandType(p,3));
	PutTxOperandFlags(p,2,GetTxOperandFlags(p,3));
	if(DBdebug(2,XINCONV))
		prafter(GetTxPrevNode(p),GetTxNextNode(p),0);
	return;
	
 maptst:
	if(DBdebug(2,XINCONV))
		prbefore(GetTxPrevNode(p),GetTxNextNode(p),0,
				"genin: map to cmp");
	PutTxOpCodeX(p,G_CMP);
	PutTxOperandAd(p,1,i0);
	PutTxOperandType(p,1,Tword);
	PutTxOperandFlags(p,1,0);	
	if(DBdebug(2,XINCONV))
		prafter(GetTxPrevNode(p),GetTxNextNode(p),0);
}
	STATIC void
genout(p)			/* Map from internal generics to 32100 code */
register TN_Id p;		/* Id for scanning text nodes. */

{
 extern int TySize();		/* Returns the byte size of a given type. */
 extern unsigned int Max_Ops;
 AN_Id adp1;			/* Id of 1st src address. */
 AN_Id adp2;			/* Id of 2nd src address. */
 AN_Id adp3;			/* Id of dst address. */
 unsigned int i;		/* Operand number. */
 extern AN_Id i0;
 extern AN_Id i1;
 extern boolean islivecc();	/* TRUE is cond codes live after node */
 int max2;			/* Maximum operand size. */ 
 unsigned int o;		/* New opcode. */
 unsigned int oo;		/* Old opcode.	*/
 TN_Id pnext;			/* Id of next text node for promoting types. */
 extern unsigned int praddr();
 extern void prafter();
 extern void prbefore();
 int ty;			/* Operand size. */
 OperandType ty1;		/* Type of Operand 1. */
 OperandType ty2;		/* Type of Operand 2. */

 oo = GetTxOpCodeX(p);
 switch(oo) 				/* Commute source operands to */
					/* create dyadics. */
	{case G_ADD3:			/* Consider commutative */
	 case G_AND3:			/* operations. */
	 case G_MFADD3:
	 case G_MFMUL3:
	 case G_MUL3:
	 case G_OR3:
	 case G_XOR3:
		if(Xskip(XOUTCONV))	/* If we are not optimizing, */
			break;		/* skip this.	*/
		adp1 = GetTxOperandAd(p,1); /* Get addresses. */
		adp2 = GetTxOperandAd(p,2);
		adp3 = GetTxOperandAd(p,3);
					/* Can this be converted to */
					/* dyadic? */
		if(adp1 != adp3 || adp2 == adp3)
			break;		/* No, do nothing. */
#ifdef W32200
					/* Auto addresses a problem? */
		if(IsAutoDependency(adp1,adp2,NULL))
			break;		/* Yes, do nothing. */
		if(IsAutoDependency(adp2,adp1,NULL))
			break;		/* Yes, do nothing. */
#endif 

					/* Okay to flip operands. */
		ty1 = GetTxOperandType(p,1);
		ty2 = GetTxOperandType(p,2);
		PutTxOperandType(p,1,ty2);
		PutTxOperandAd(p,1,adp2);
		PutTxOperandType(p,2,ty1);
		PutTxOperandAd(p,2,adp1);
		endcase;
	 default:			/* Do Nothing. */
		endcase;
	}

 switch( oo )				/* promote halfwords and */
					/* bytes to words. 	*/
	{case G_ADD3:
	 case G_ALS3:
	 case G_AND3:
	 case G_LLS3:
	 case G_OR3:
	 case G_SUB3:
	 case G_XOR3:
		if(Xskip(XOUTCONV))	/* If not optimizing, */
			break;		/* skip this. */
		adp3 = GetTxOperandAd(p,3);
		if(!IsAdCPUReg(adp3))	/* dst is a cpu register */
			endcase;
		if(islivecc(p)) 	/* cond codes not needed */
			endcase;
		pnext = GetTxNextNode(p);
		if(pnext == NULL 
				|| !IsOpGeneric(GetTxOpCodeX(pnext))
				|| IsOpDstSrc(GetTxOpCodeX(pnext)))
						/* next instruction is */
						/* generic with pure dst */
			endcase;
		if(!(IsDeadAd(adp3,pnext)
				|| adp3 == GetTxOperandAd(pnext,3)))
						/* dst is dead after next */
						/* instruction or set by next */
						/* instruction */
			endcase;
						/* find max size use of dst */
		max2 = 0;
		for(i = 0; i < Max_Ops; i++)
			{if(GetTxOperandAd(pnext,i) == adp3 )
				{if(i == 3) break;
				 ty = TySize(GetTxOperandType(pnext,i));
				 max2 = ( ty > max2 ) ? ty : max2;
				}
			 else if(IsAdUses(GetTxOperandAd(pnext,i),adp3))
				max2 = TySize(Tword);
			}
				
						/* check if can promote */
		ty1 = GetTxOperandType(p,1);
		ty2 = GetTxOperandType(p,2);
		if((ty1 != Tword) && !IsAdCPUReg(GetTxOperandAd(p,1)))
			endcase;
		if((ty2 != Tword) && !IsAdCPUReg(GetTxOperandAd(p,2)))
			endcase;
		if((max2 > TySize(ty1)) || (TySize(ty1)>=TySize(Tword)))
			endcase;
		if((max2 > TySize(ty2)) || (TySize(ty2)>=TySize(Tword)))
			endcase;
						/* Yes.  Do it. */
		if(DBdebug(3,XOUTCONV)){
			prbefore(GetTxPrevNode(p),GetTxNextNode(p),0,"genout");
			(void)fprintf(stdout,"%cpromotion to words ",ComChar);
			(void)praddr(GetTxOperandAd(p,1),ty1,stdout);
			putchar(SPACE);
			(void)praddr(GetTxOperandAd(p,2),ty2,stdout);
			putchar(NEWLINE);
		}
		PutTxOperandType(p,1,Tword);
		PutTxOperandType(p,2,Tword);
		endcase;
	}
	
 switch(oo)
	{ 
	 case G_ADD3:	o = G_ADD2;	goto maptri;
	 case G_AND3:	o = G_AND2; 	goto maptri;
	 case G_DIV3:	o = G_DIV2; 	goto maptri;
	 case G_MOD3:	o = G_MOD2; 	goto maptri;
	 case G_MUL3:	o = G_MUL2; 	goto maptri;
	 case G_OR3:	o = G_OR2;	goto maptri;
	 case G_SUB3:	o = G_SUB2; 	goto maptri;
	 case G_XOR3:	o = G_XOR2; 	goto maptri;
	 case G_MFABS2:	o = G_MFABS1;	goto mapdy;
	 case G_MFADD3:	o = G_MFADD2;	goto maptri;
	 case G_MFDIV3:	o = G_MFDIV2;	goto maptri;
	 case G_MFMUL3:	o = G_MFMUL2;	goto maptri;
	 case G_MFNEG2:	o = G_MFNEG1;	goto mapdy;
	 case G_MFREM3:	o = G_MFREM2;	goto maptri;
	 case G_MFRND2:	o = G_MFRND1;	goto mapdy;
	 case G_MFSQR2:	o = G_MFSQR1; 	goto mapdy;
	 case G_MFSUB3:	o = G_MFSUB2; 	goto maptri;
	 default: goto optinst;
	} /* END OF switch(GetTxOpCodeX(p)) */

 mapdy:
	if((GetTxOperandAd(p,2) == GetTxOperandAd(p,3)) &&
			(GetTxOperandType(p,2) == GetTxOperandType(p,3)))
	{
		if(DBdebug(2,XOUTCONV))
			prbefore(GetTxPrevNode(p),GetTxNextNode(p),0,
				"genout: map to monadic");
		PutTxOpCodeX(p,o);
		PutTxOperandAd(p,2,i0);
		PutTxOperandType(p,2,Tword);
		if(DBdebug(2,XOUTCONV))
			prafter(GetTxPrevNode(p),GetTxNextNode(p),0);
	}
	goto mapcpu;
	
 maptri:
	if((GetTxOperandAd(p,2) == GetTxOperandAd(p,3)) &&
			(GetTxOperandType(p,2) == GetTxOperandType(p,3)))
	{
		if(DBdebug(2,XOUTCONV))
			prbefore(GetTxPrevNode(p),GetTxNextNode(p),0,
				"genout: map to dyadic");
		PutTxOpCodeX(p,o);
		PutTxOperandAd(p,2,GetTxOperandAd(p,1));
		PutTxOperandType(p,2,GetTxOperandType(p,1));
		PutTxOperandAd(p,1,i0);
		PutTxOperandType(p,1,Tword);
		if(DBdebug(2,XOUTCONV))
			prafter(GetTxPrevNode(p),GetTxNextNode(p),0);
	}
	
 optinst:
	switch(GetTxOpCodeX(p))
		{case G_ADD2:	o = G_INC;	goto mapadd;
		 case G_CMP:	o = G_TST;	goto mapcmp;
		 case G_MOV:	o = G_CLR;	goto mapmov;
		 case G_SUB2:	o = G_DEC;	goto mapsub;
		 default: endcase;
		}
	goto mapcpu;

 mapadd:
 mapsub:
	if(GetTxOperandAd(p,2) == i1)
	{
		if(DBdebug(2,XOUTCONV))
			prbefore(GetTxPrevNode(p),GetTxNextNode(p),0,
				"genout: add/sub conversion");
		PutTxOpCodeX(p,o);
		PutTxOperandAd(p,2,i0);
		PutTxOperandType(p,2,Tword);
		if(DBdebug(2,XOUTCONV))
			prafter(GetTxPrevNode(p),GetTxNextNode(p),0);
	}
	goto mapcpu;
	
 mapcmp:
	if(GetTxOperandAd(p,1) == i0)
	{
		if(DBdebug(2,XOUTCONV))
			prbefore(GetTxPrevNode(p),GetTxNextNode(p),0,
				"genout: cmp conversion");
		PutTxOpCodeX(p,o);
		PutTxOperandAd(p,1,i0);
		PutTxOperandType(p,1,Tword);
		if(DBdebug(2,XOUTCONV))
			prafter(GetTxPrevNode(p),GetTxNextNode(p),0);
	}
	goto mapcpu;

 mapmov:
	if(GetTxOperandAd(p,2) == i0)
	{
		if(DBdebug(2,XOUTCONV))
			prbefore(GetTxPrevNode(p),GetTxNextNode(p),0,
				"genout: mov conversion");
		PutTxOpCodeX(p,o);
		PutTxOperandAd(p,2,i0);
		PutTxOperandType(p,2,Tword);
		if(DBdebug(2,XOUTCONV))
			prafter(GetTxPrevNode(p),GetTxNextNode(p),0);
	}

 mapcpu:	
	frgen(p);

}
	STATIC void
Bconv(p)				/* Converts cpu branches into 
					 * IS25 jmps. This is necessary (for now)
					 * since cpu branches are pc relative and 
					 * PCI is oblivious to distance requirements.
					 */
TN_Id p;
{
 register unsigned short op;
 extern void fatal();

 op = GetTxOpCodeX(p);
 if(IsOpIs25(op))	/* already IS25. */
	return;
 switch(op){
 case BCCB:
 case BCCH:
 case BGEUB:
 case BGEUH:
	op = IJGEU;
	endcase;
 case BCSB:
 case BCSH:
 case BLUB:
 case BLUH:
	op = IJLU;
	endcase;
 case BEB:
 case BEH:
	op = IJE;
	endcase;
 case BGB:
 case BGH:
	op = IJG;
	endcase;
 case BGEB:
 case BGEH:
	op = IJGE;
	endcase;
 case BGUB:
 case BGUH:
	op = IJGU;
	endcase;
 case BLB:
 case BLH:
	op = IJL;
	endcase;
 case BLEB:
 case BLEH:
	op = IJLE;
	endcase;
 case BLEUB:
 case BLEUH:
	op = IJLEU;
	endcase;
 case BNEB:
 case BNEH:
	op = IJNE;
	endcase;
 case BRB:
 case BRH:
	op = IJMP;
	endcase;
 case BSBB:
 case BSBH:
	op = IJSB;
	endcase;
 case BVCB:
 case BVCH:
 case BVSB:
 case BVSH:		/* no equivalents for these	*/
	fatal("Bconv: overflow set/clear branches not supported.\n");
	endcase;
 case RET:
 case JMP:		/* not pc-relative, o.k.	*/
	return;
 default:
	fatal("Bconv: unrecognized branch opcode (%d)\n", op);
	endcase;
 }
 PutTxOpCodeX(p, op);	/* rewrite it */
}
	STATIC void
i25in(p)				/* Convert IS25 input to 32100
					 * instructions, then to generics.
					 */
TN_Id p;
{
 extern void fatal();			/* Handles fatal errors.	*/
 unsigned char flags;			/* Temp to hold flags for swap. */
 AN_Id hopnd;				/* Temp to operand AN Id. */
 extern AN_Id i0;			/* Address-node-id of Immede 0.	*/
 unsigned short newop;			/* new opcode index. */
 unsigned short oldop;			/* Old opcode index. */
 extern struct opent optab[];		/* The operation-code table.	*/
 register TN_Id pn,p2;			/* Text node pointers. */
 AN_Id tmppn;				/* Temporary address node index. */
 OperandType type;			/* Operand type. */
 long int wdsize;			/* Word size. */

 if(IsTxBr(p))			/* jmps and rets remain untouched. */
	return;

 oldop = GetTxOpCodeX(p);
					/* Found a 1 to 1 mapping.	*/
 if((int) (optab[oldop].oopcode) != OLOWER) 
	{PutTxOpCodeX(p,(unsigned short) optab[oldop].oopcode);
	 goto maptogen;
	}

					/* Do specialized mappings. */
 switch((int) oldop)
	{
		 		/* iacjl* -->	ADDW2	incr, index   */
		 		/* 		CMPW	index, limit  */
		 		/* 		jl*	dest	      */
		 
					/* Choose new jump type-- */
					/* third instr of sequence */
	 case IACJL:	newop = IJL; goto acj;
	 case IACJLE:	newop = IJLE; goto acj;
	 case IACJLEU:	newop = IJLEU; goto acj;
	 case IACJLU:	newop = IJLU; goto acj;
					/* Make second instr -- CMPW */
	 acj:
		pn = MakeTxNodeAfter(p,CMPW);
		PutTxOperandAd(pn,0,GetTxOperandAd(p,2));
		PutTxOperandType(pn,0,GetTxOperandType(p,2));
		PutTxOperandFlags(pn,0,GetTxOperandFlags(p,2));
		PutTxOperandAd(pn,1,GetTxOperandAd(p,0));
		PutTxOperandType(pn,1,GetTxOperandType(p,0));
		PutTxOperandFlags(pn,1,GetTxOperandFlags(p,0));
					/* Make third instr of sequence */
		p2 = MakeTxNodeAfter(pn,newop);
		PutTxOperandAd(p2,0,GetTxOperandAd(p,3)); 
		PutTxOperandType(p2,0,Tubyte);
		PutTxOperandFlags(p2,0,GetTxOperandFlags(p,3));
					/* Change existing instr node to */
					/* first instr -- ADDW2 */
		PutTxOpCodeX(p,ADDW2);
		hopnd = GetTxOperandAd(p,0);
		type = GetTxOperandType(p,0);
		flags = GetTxOperandFlags(p,0);
		PutTxOperandAd(p,0,GetTxOperandAd(p,1));
		PutTxOperandType(p,0,GetTxOperandType(p,1));
		PutTxOperandFlags(p,0,GetTxOperandFlags(p,1));
		PutTxOperandAd(p,1,hopnd);
		PutTxOperandType(p,1,type);
		PutTxOperandFlags(p,1,flags);
		PutTxOperandAd(p,2,i0);
		PutTxOperandFlags(p,2,0);
		PutTxOperandAd(p,3,i0);
		PutTxOperandFlags(p,3,0);
		endcase;

		 		/* iatjnz* -->	ADDW2	incr, index  */
		 		/* 		TST*	*index       */
				/*		jnz	dest         */

					/* Select data type for TST* and */
					/* operand */
	 case IATJNZB:	newop = TSTB; type = Tbyte; goto atj;
	 case IATJNZH:	newop = TSTH; type = Thalf; goto atj;
	 case IATJNZW:	newop = TSTW; type = Tword; goto atj;
					/* make second instr of sequence -- */
					/* TST[bhw] */
	 atj:
		pn = MakeTxNodeAfter(p,newop);
		PutTxOperandType(pn,0,type);
		tmppn = GetTxOperandAd(p,0);
		flags = GetTxOperandFlags(p,0);
		PutTxOperandAd(pn,0,
			GetAdAddIndInc(GetTxOperandType(p,0),
				type,tmppn,0));
		PutTxOperandFlags(pn,0,0);
		if(IsTxOperandVol(p,0))	/* inherit volatility */
			PutTxOperandVol(pn,0,TRUE);
					/* make last instr in sequence --  */
					/* IJNZ */
		p2 = MakeTxNodeAfter(pn, IJNZ);
		PutTxOperandType(p2,0,GetTxOperandType(p,2));
		PutTxOperandAd(p2,0,GetTxOperandAd(p,2));
		PutTxOperandFlags(p2,0,GetTxOperandFlags(p,2));
					/* change existing instr into ADDW2 */
		PutTxOpCodeX(p, ADDW2);
		PutTxOperandAd(p,0,GetTxOperandAd(p,1));
		PutTxOperandFlags(p,0,GetTxOperandFlags(p,1));
		PutTxOperandAd(p,1,tmppn);
		PutTxOperandFlags(p,1,flags);
		PutTxOperandAd(p,2,i0);
		PutTxOperandFlags(p,2,0);
		endcase;

		 	/* cmp* op1, op2  --->  CMP* op2, op1  */
		
					/* choose new instruction */
	 case ICMPB:	PutTxOpCodeX(p,CMPB); goto cmp;
	 case ICMPH:	PutTxOpCodeX(p,CMPH); goto cmp;
	 case ICMPW:	PutTxOpCodeX(p,CMPW); goto cmp;
					/* swap address nodes */
	 cmp:
		hopnd = GetTxOperandAd(p,0);
		PutTxOperandAd(p,0,GetTxOperandAd(p,1));
		PutTxOperandAd(p,1,hopnd);
					/* swap types */
		type = GetTxOperandType(p,0);
		PutTxOperandType(p,0,GetTxOperandType(p,1));
		PutTxOperandType(p,1,type);
					/* swap flags */
		flags = GetTxOperandFlags(p,0);
		PutTxOperandFlags(p,0,GetTxOperandFlags(p,1));
		PutTxOperandFlags(p,1,flags);
		endcase;

	 case IMOVBBH:
		PutTxOpCodeX(p, MOVB);
		PutTxOperandType(p,0,Tsbyte);
		endcase;
	 case IMOVBBW:
		PutTxOpCodeX(p, MOVB);
		PutTxOperandType(p,0,Tsbyte);
		endcase;
	 case IMOVBHW:
		PutTxOpCodeX(p, MOVH);
		endcase;
	 case IMOVZBH:
		PutTxOpCodeX(p, MOVB);
		PutTxOperandType(p,1,Tuhalf);
		endcase;
	 case IMOVZBW:
		PutTxOpCodeX(p, MOVB);
		PutTxOperandType(p,1,Tuword);
		endcase;
	 case IMOVZHW:
		PutTxOpCodeX(p, MOVH);
		PutTxOperandType(p,0,Tuhalf);
		PutTxOperandType(p,1,Tuword);
		endcase;
	 case IMOVTHB: 
		if(IsAdCPUReg(GetTxOperandAd(p,1)))
			{PutTxOpCodeX(p, ANDH3);
			 PutTxOperandAd(p,2,GetTxOperandAd(p,1));
			 PutTxOperandType(p,2,Tbyte);
			 PutTxOperandFlags(p,2,GetTxOperandFlags(p,1));
			 PutTxOperandAd(p,1,GetTxOperandAd(p,0));
			 PutTxOperandType(p,1,GetTxOperandType(p,0));
			 PutTxOperandFlags(p,1,GetTxOperandFlags(p,0));
			 PutTxOperandAd(p,0,GetAdImmediate("0xff"));
			 PutTxOperandFlags(p,0,0);
			}
		else
			{PutTxOpCodeX(p, MOVH);
			 PutTxOperandType(p,1,Tsbyte);
			}
		endcase;
	 case IMOVTWB:
		if(IsAdCPUReg(GetTxOperandAd(p,1)))
			{PutTxOpCodeX(p, ANDW3);
			 PutTxOperandAd(p,2,GetTxOperandAd(p,1));
			 PutTxOperandType(p,2,Tbyte);
			 PutTxOperandFlags(p,2,GetTxOperandFlags(p,1));
			 PutTxOperandAd(p,1,GetTxOperandAd(p,0));
			 PutTxOperandType(p,1,GetTxOperandType(p,0));
			 PutTxOperandFlags(p,1,GetTxOperandFlags(p,0));
			 PutTxOperandAd(p,0,GetAdImmediate("0xff"));
			 PutTxOperandFlags(p,0,0);
			}
		else
			{PutTxOpCodeX(p, MOVW);
			 PutTxOperandType(p,1,Tsbyte);
			}
		endcase;
	 case IMOVTWH:
		if(IsAdCPUReg(GetTxOperandAd(p,1)))
			{PutTxOpCodeX(p,MOVW);
			 pn = MakeTxNodeAfter(p,MOVH);
			 PutTxOperandAd(pn,0,GetTxOperandAd(p,1));
			 PutTxOperandType(pn,0,Thalf);
			 PutTxOperandFlags(pn,0,GetTxOperandFlags(p,1));
			 PutTxOperandAd(pn,1,GetTxOperandAd(p,1));
			 PutTxOperandType(pn,1,Thalf);
			 PutTxOperandFlags(pn,1,GetTxOperandFlags(p,1));
			}
		else
			{PutTxOpCodeX(p, MOVW);
			 PutTxOperandType(p,1,Thalf);
			}
		endcase;
		
					/* push[zb][bh] is expanded into 	*/
					/*    ADDW2 &4, %sp 			*/
					/* followed by a second instruction--	*/
					/* if opnd doesn't use %sp, then 	*/
					/*    MOVX{Z}opnd,{sword}-4(%sp) 	*/
					/* if opnd = %sp, then 			*/
					/*    SUBW3 &4,{Z}opnd,{sword}-4(%sp) 	*/
					/* or if opnd=expr(%sp), then 		*/
					/*    MOVX {Z}expr-4(%sp),	 	*/
					/*		{sword}-4(%sp) 		*/
					/* where Z is sbyte or shalf 		*/
					/*    depending on X 			*/
	
					/* select new opcode and operand type */
	 case IPUSHZB:	type = Tbyte; newop = MOVB; goto pushzb;
	 case IPUSHZH:	type = Tuhalf; newop = MOVH; goto pushzb;
	 case IPUSHBB:	type = Tsbyte; newop = MOVB; goto pushzb;
	 case IPUSHBH:	type = Thalf; newop = MOVH; goto pushzb;
	 pushzb:
		wdsize = TySize(Tword);
					/* Does operand use %sp ? */
		if(!IsAdUses(GetTxOperandAd(p,0),GetAdCPUReg(CSP)))
			{	/* operand doesn't use %sp */
			 pn = MakeTxNodeAfter(p,newop);
			 PutTxOperandAd(pn,0,GetTxOperandAd(p,0));
			 PutTxOperandType(pn,0,type);
			 PutTxOperandFlags(pn,0,GetTxOperandFlags(p,0));
			 PutTxOperandAd(pn,1,
				GetAdDispInc(Tword,"",CSP,-wdsize));
			 PutTxOperandType(pn,1,Tword);
			 PutTxOperandFlags(pn,1,0);
			} 
					/* is operand == %sp ? */
		else if(GetTxOperandAd(p,0)==GetAdCPUReg(CSP))
			{	/* operand == %sp */
			 pn=MakeTxNodeAfter(p,SUBW3);
			 PutTxOperandType(pn,0,Tword);
			 PutTxOperandAd(pn,0,GetAdAddToKey(Tword,i0,wdsize));
			 PutTxOperandFlags(pn,0,0);
			 PutTxOperandType(pn,1,type);
			 PutTxOperandAd(pn,1,GetTxOperandAd(p,0));
			 PutTxOperandFlags(pn,1,GetTxOperandFlags(p,0));
			 PutTxOperandType(pn,2,Tword);
			 PutTxOperandAd(pn,2,
				GetAdDispInc(type,"",CSP,-wdsize));
			 PutTxOperandFlags(pn,2,0);
			}
		else
			{ 	/* operand == expr(%sp) */
			 pn=MakeTxNodeAfter(p,newop);
			 PutTxOperandType(pn,0,type);
			 tmppn = GetTxOperandAd(p,0);
			 PutTxOperandAd(pn,0,
				GetAdAddToKey(type,tmppn,-wdsize));
			 PutTxOperandFlags(pn,0,GetTxOperandFlags(p,0));
			 PutTxOperandType(pn,1,Tword);
			 PutTxOperandAd(pn,1,
				GetAdDispInc(Tword,"",CSP,-wdsize));
			 PutTxOperandFlags(pn,1,0);
			}
					/* convert existing instr to ADDW2 */
		PutTxOpCodeX(p,ADDW2);
		PutTxOperandAd(p,0,GetAdAddToKey(Tword,i0,wdsize));
		PutTxOperandType(p,0,Tword);
		PutTxOperandFlags(p,0,0);
		PutTxOperandAd(p,1,GetAdCPUReg(CSP));
		PutTxOperandType(p,1,Tword);
		PutTxOperandFlags(p,1,0);
		endcase;

	 case IALSW2:
		PutTxOperandAd(p,2,GetTxOperandAd(p,1));
		PutTxOperandType(p,2,GetTxOperandType(p,1));
		PutTxOperandFlags(p,2,GetTxOperandFlags(p,1));
		/* FALLTHRU */
	 case IALSW3:
					/* direct mapping unless count */
					/* operand is a register */
			 		/* or an immediate then */
			 		/* alsw3 count,src,dest changes into */
			   		/* ALSW3 {sbyte}count,src,dest */
		tmppn = GetTxOperandAd(p,0);
		if(IsAdCPUReg(tmppn) || IsAdImmediate(tmppn))
			PutTxOperandType(p,0,Tword);
		PutTxOpCodeX(p,ALSW3);
		endcase;
	 case IARSW2:
		PutTxOperandAd(p,2,GetTxOperandAd(p,1));
		PutTxOperandType(p,2,GetTxOperandType(p,1));
		PutTxOperandFlags(p,2,GetTxOperandFlags(p,1));
		/* FALLTHRU */
	 case IARSW3:
					/* direct mapping unless count */
					/* operand is a register */
			 		/* or an immediate then */
			 		/* arsw3  count,src,dest  chgs to */
					/* ARSB3 count,src,dest */
		tmppn = GetTxOperandAd(p,0);
		if(IsAdCPUReg(tmppn) || IsAdImmediate(tmppn))
			{PutTxOpCodeX(p,ARSW3);
			 PutTxOperandType(p,0,Tword);
			}
		else 
			{PutTxOpCodeX(p,ARSB3);
			 PutTxOperandType(p,0,Tbyte);
			}
		endcase;
	 case ILLSW2: 
		PutTxOperandType(p,2,GetTxOperandType(p,1));
		PutTxOperandAd(p,2,GetTxOperandAd(p,1));
		PutTxOperandFlags(p,2,GetTxOperandFlags(p,1));
		/* FALLTHRU */
	 case ILLSW3:
					/* direct mapping unless count */
					/* operand is a register */
			 		/* or an immediate then */
			 		/* llsw3 count,src,dest chgs to */
					/* LLSB3 count,src,dest */
		tmppn = GetTxOperandAd(p,0);
		if(IsAdCPUReg(tmppn) || IsAdImmediate(tmppn))
			{PutTxOpCodeX(p,LLSW3);
			 PutTxOperandType(p,0,Tword);
			}
		else
			{PutTxOpCodeX(p,LLSB3);
			 PutTxOperandType(p,0,Tbyte);
			}
		endcase;
	 case ILRSW2:
		PutTxOperandType(p,2,GetTxOperandType(p,1));
		PutTxOperandAd(p,2,GetTxOperandAd(p,1));
		PutTxOperandFlags(p,2,GetTxOperandFlags(p,1));
		/* FALLTHRU */
	 case ILRSW3:
					/* direct mapping unless count */
					/* operand is a register */
			 		/* or an immediate then */
			 		/* lrsw3 count,src,dest chgs to */
					/* LRSW3 {sbyte}count,src,dest */
		tmppn = GetTxOperandAd(p,0);
		if(IsAdCPUReg(tmppn) || IsAdImmediate(tmppn))
			PutTxOperandType(p,0,Tword);
		PutTxOpCodeX(p,LRSW3);
		endcase;
					/* insv src,offset,width,dest  ---> */
					/* INSFW width-1,offset,src,dest */
	 case IINSV:
		PutTxOpCodeX(p, INSFW);
		hopnd = GetTxOperandAd(p,0);
		flags = GetTxOperandFlags(p,0);
		tmppn = GetTxOperandAd(p,2);
		PutTxOperandAd(p,0,
			GetAdAddToKey(GetTxOperandType(p,0),tmppn,(-1)));
		PutTxOperandFlags(p,0,GetTxOperandFlags(p,2));
		PutTxOperandAd(p,2,hopnd);
		PutTxOperandFlags(p,2,flags);
		endcase;
					/* extzv offset,width,src,dest ---> */
					/* EXTFW width-1,offset,src,dest */
	 case IEXTZV:
		PutTxOpCodeX(p, EXTFW);
		hopnd = GetTxOperandAd(p,0);
		flags = GetTxOperandFlags(p,0);
		tmppn = GetTxOperandAd(p,1);
		PutTxOperandAd(p,0,
			GetAdAddToKey(GetTxOperandType(p,0),tmppn,(-1)));
		PutTxOperandFlags(p,0,GetTxOperandFlags(p,1));
		PutTxOperandAd(p,1,hopnd);
		PutTxOperandFlags(p,1,flags);
		endcase;
	 case IMOVBLB:		/* not implemented */
		fatal("i25in: opcode not implemented\n");
		/*NOTREACHED*/
		endcase;
	 case IMOVBLH: 		/* not implemented */
		fatal("i25in: opcode not implemented\n");
		/*NOTREACHED*/
		endcase;
	 case IFADDD2:
	 case IFADDD3:
	 case IFADDS2:
	 case IFADDS3:
	 case IFCMPD:
	 case IFCMPS:
	 case IFDIVD2:
	 case IFDIVD3:
	 case IFDIVS2:
	 case IFDIVS3:
	 case IFMULD2:
	 case IFMULD3:
	 case IFMULS2:
	 case IFMULS3:
	 case IFSUBD2:
	 case IFSUBD3:
	 case IFSUBS2:
	 case IFSUBS3:
	 case IMOVDD:
	 case IMOVDH:
	 case IMOVDS:
	 case IMOVDW:
	 case IMOVHD:
	 case IMOVHS:
	 case IMOVSD:
	 case IMOVSH:
	 case IMOVSS:
	 case IMOVSW:
	 case IMOVTDH:
	 case IMOVTDW:
	 case IMOVTSH:
	 case IMOVTSW:
	 case IMOVWD:
	 case IMOVWS:
		fatal("is25in: floating point is25 not supported\n");
		/*NOTREACHED*/
		endcase;
	 default:
				/* All other is25 opcodes pass */
				/* through unchanged */
		endcase;
	} /* END OF switch((int) oldop) */

 maptogen:
	genin(p);		/* convert to internal generics */
 return;
}
	STATIC void
i25out(p)				/* Convert save and return */
					/* is25 instructions to 32100 */
					/* instructions. */
TN_Id p;
{
 extern void fatal();			/* Handles fatal errors.	*/
 register int num;			/* CPU register number. */
 extern void prafter();
 extern void prbefore();
 AN_Id tmpad;				/* Temporary address index. */

 if(IsTxCBr(p)) 
	return;
					/* Do specialized mappings. */
 switch(GetTxOpCodeX(p))
	{
	 case ISAVE:
					/* save num chgs to SAVE %rm */
					/* where n=(9-num). */
		if(DBdebug(2,XOUTCONV))
			prbefore(GetTxPrevNode(p),GetTxNextNode(p),0,"i25out");
		PutTxOpCodeX(p,SAVE);
		tmpad = GetTxOperandAd(p,0);
		if(IsAdImmediate(tmpad))
			{num = 9 - GetAdNumber(GetTxOperandType(p,0),tmpad);
			 if((num < 3) || (num > 9))
				fatal("i25out: Bad ISAVE arg (%ld).\n",num);
			 tmpad=GetAdCPUReg(GetRegId(num));
			 PutTxOperandAd(p,0,tmpad);
			}
		else
			fatal("i25out: illegal addr mode on SAVE.\n");
		if(DBdebug(2,XOUTCONV))
			prafter(GetTxPrevNode(p),GetTxNextNode(p),0);
		endcase;
	 case IRET:
					/* ret num chgs to RESTORE %rn; RET */
					/* where n=(9-num) */
		if(DBdebug(2,XOUTCONV))
			prbefore(GetTxPrevNode(p),GetTxNextNode(p),0,"i25out");
		PutTxOpCodeX(p,RESTORE);
		tmpad = GetTxOperandAd(p,0);
		if(IsAdImmediate(tmpad))
			{num = 9 - GetAdNumber(GetTxOperandType(p,0),tmpad);
			 if((num < 3) || (num > 9))
				fatal("i25out: Bad IRET arg (%ld).\n",num);
			 tmpad=GetAdCPUReg(GetRegId(num));
			 PutTxOperandAd(p,0,tmpad);
			}
		else
			fatal("i25out: illegal addr mode for IRET.\n");
		p = MakeTxNodeAfter(p,RET);
		if(DBdebug(2,XOUTCONV))
			prafter(GetTxPrevNode(GetTxPrevNode(p)),GetTxNextNode(p),0);
		endcase;
	 default:
					/* All other is25 opcodes pass */
					/* through unchanged */
		endcase;
	} /* END OF switch((int) oldop) */
 return;
}
	STATIC boolean
togen(p)			/* Convert a nongeneric with an associated
				 * generic opcode, to that generic.
				 */
register TN_Id p;
{extern unsigned int Max_Ops;	/* Maximum number of operands.	*/
 AN_Id abuf[4];
 unsigned char fbuf[4];
 unsigned short gop;
 extern AN_Id i0;		/* AN_Id of immediate 0.	*/
 register int j;
 register unsigned int operand;	/* Operand counter.	*/
 extern struct opent optab[];	/* The operation-code table.	*/
 OperandType tbuf[4];

 gop = optab[GetTxOpCodeX(p)].oopcode;
 if(!IsOpGeneric(gop))			/* Is there an associated generic? */
	return(FALSE);			/* No. */
		
 PutTxOpCodeX(p,gop);			/*  Change opcode.	*/

					/* Shift operands.	*/
 for(operand = 0; operand < Max_Ops; operand++)
	{abuf[operand] = GetTxOperandAd(p,operand);
	 tbuf[operand] = GetTxOperandType(p,operand);
	 fbuf[operand] = GetTxOperandFlags(p,operand);
	}
 j = 0;
 for(operand = 0; operand < Max_Ops; operand++)
	{if(optab[gop].otype[operand] == TNONE )
		{PutTxOperandAd(p,operand,i0);
		 PutTxOperandType(p,operand,Tword);
		 PutTxOperandFlags(p,operand,0);
		 continue;
		}
	 else
		{PutTxOperandAd(p,operand,abuf[j]);
		 PutTxOperandType(p,operand,tbuf[j]);
		 PutTxOperandFlags(p,operand,fbuf[j]);
		 j++;
		}
	} /* END OF for(operand = 0; operand < Max_Ops; operand++) */
 return(TRUE);
}
	STATIC void
frgen(p)		 	/* Convert from generics to cpu.	*/

register TN_Id p;
{extern unsigned int Max_Ops;	/* Maximum operands in an instruction.	*/
 boolean allmatch;		/* True if all operands of a nongen instr matches */
 extern void fatal();		/* Prints fatal messages. */
 boolean firstop;		/* True if we're at the first operand */
 extern AN_Id i0;		/* AN_Id of immediate 0. */
 register unsigned int lastopn;
 unsigned short op;
 unsigned short op1match;	/* Opcode of first nongen instr with an operand match */
 unsigned short opnon;
 extern void prafter();
 extern void prbefore();
 OperandType t;
 OperandType tnon;
 OperandType tp1;
 register unsigned int operand;	/* Operand counter.	*/

 if(DBdebug(2,XOUTCONV))
	prbefore(GetTxPrevNode(p),GetTxNextNode(p),0,
			"frgen: map to cpu instruction");
			
 op = GetTxOpCodeX(p);
						/* Shift operands left.	*/
 lastopn = 0;
 for(operand = 0; operand < Max_Ops; operand++)
	{if(optab[op].otype[operand] == TNONE)
		continue;
	 else
		{PutTxOperandAd(p,lastopn,GetTxOperandAd(p,operand));
		 PutTxOperandType(p,lastopn,GetTxOperandType(p,operand));
			 lastopn++;
		}
	} 
						/* Fill the rest with &0. */
 for(operand = lastopn; operand < Max_Ops; operand++)
	{PutTxOperandAd(p,operand,i0);
	 PutTxOperandType(p,operand,Tnone);
	}

		/* If a nongeneric opcode matches all operand types, use it.
		 * Otherwise, use the first opcode with an operand type match.
		 * (we actually look at just the first operand, is this o.k.? -RTL)
		 * Since the opcodes are arranged in wide-to-narrow type order,
		 * we're safe. If all else fails, use the default.
		 */
 allmatch = FALSE;
 op1match = NULL;
 for(opnon = optab[op].onongens;
		opnon != OLOWER; 
			opnon = optab[opnon].onongens)
	{allmatch = TRUE;
	 firstop = TRUE;
	 for(operand = 0; operand < lastopn; operand++)
		{t = GetTxOperandType(p,operand);
		 tnon = (OperandType) optab[opnon].otype[operand];
		 if(IsTxCPUOpc(p) && 
				IsAdImmediate(GetTxOperandAd(p,
				operand)) && 
				(tp1 = GetTxOperandType(p,operand + 1))
				!= Tnone &&
				IsSigned(t) == IsSigned(tp1)) 
			continue;		/* immediate constants */
						/* control own length */
						/* on CPU instructions */
		 if( t != tnon ) 
			{allmatch = FALSE;
			 break;
			}
		 else if(firstop){
			if(op1match == NULL)
				op1match = opnon;
			firstop = FALSE;
			}
		} /* END OF for(operand = 0; operand < lastopn; ... */
	 if(allmatch)
		break;
	} /* END OF for(opnon = optab[op].onongens; ... */
 if(allmatch)
	PutTxOpCodeX(p,opnon);
 else if(op1match != NULL)
	PutTxOpCodeX(p,op1match);
 else if((opnon = optab[op].oopcode) != OLOWER)
 	PutTxOpCodeX(p,opnon);
 else 
	fatal("frgen: can't map generic for output - '%s'\n",optab[op].oname);
			
 if(DBdebug(2,XOUTCONV))
	prafter(GetTxPrevNode(p),GetTxNextNode(p),0);

 return;
}
	STATIC TN_Id
cputomis(p)			/* Convert CPU moves (following a #TYPE) 
				 * to MIS moves. All moves will be removed
				 * except one with the lowest src address.
				 */
register TN_Id p;		/* Points to the #TYPE text node.	*/

{
 AN_Id ad1, ad2;
 int c1, c2;
 char *expr;			/* Expression part of address. */
 extern void fatal();		/* Handles fatal errors.	*/
 extern AN_Id i0;		/* AN_Id of immediate 0.	*/
 extern unsigned int ndisc;	/* Number of instructions discarded.	*/
 int n; 			/* Loop index for counting words. */
 unsigned int operand;
 AN_Mode md1, md2;
 TN_Id p1, p2;
 extern void prafter();
 extern void prbefore();
 char *string1;
 char *string2;
 OperandType ty;
 long int tysize;		/* Size of operand in bytes. */
 long int wordsize;		/* Size of a word in bytes. */

 ty = GetTxOperandType(p,0);

 if(!IsFP(ty))			/* Only #TYPE [SINGLE|DOUBLE|DBLEXT] allowed */
	return(p);

 tysize = TySize(ty);
 wordsize = TySize(Tword);

 if(DBdebug(2,XINCONV)){
	for(p1 = GetTxNextNode(p), n = wordsize; n < tysize; n += wordsize)
		p1 = GetTxNextNode(p1);
	prbefore(GetTxPrevNode(p), GetTxNextNode(p1), 0, "cputomis");
	}
				/* Remove all but lowest src address instruction */
 for(n = wordsize; n < tysize; n += wordsize)
	{p1 = GetTxNextNode(p);
	 switch(GetTxOpCodeX(p1)){
	 case G_MOV:
	 case G_PUSH:		/* get src address & mode */
	 	ad1 = GetTxOperandAd(p1,2);
	 	md1 = GetAdMode(ad1);
		break;
	 default:
		fatal("cputomis: illegal instruction after #TYPE\n");
	 }
	 p2 = GetTxNextNode(p1);
	 switch(GetTxOpCodeX(p2)){
	 case G_MOV:
	 case G_PUSH:		/* get src address & mode */
	 	ad2 = GetTxOperandAd(p2,2);
	 	md2 = GetAdMode(ad2);
		break;
	 default:
		fatal("cputomis: illegal instruction after #TYPE\n");
	 }
		
	 if( md1 != md2 )
		fatal("cputomis:incompat address modes after #TYPE\n");
	 switch( md1 )
		{case CPUReg:
			c1 = GetRegNo(GetAdRegA(ad1));
			c2 = GetRegNo(GetAdRegA(ad2));
			break;
		 case Absolute:
		 case Disp:
		 case IndexRegDisp:
			expr = GetAdExpression(Tword,ad1);
			c1 = ANNormExpr(expr,&string1);
			string1 = strdup(string1);
			expr = GetAdExpression(Tword,ad2);
			c2 = ANNormExpr(expr,&string2);
			if(strcmp(string1,string2) != 0)
				fatal("cputomis:incompat addresses for #TYPE\n");
			Free(string1);
			break;
		default:
			fatal( "cputomis:illegal address mode after #TYPE\n" );
		}
	 if( c1 < c2 )
		{DelTxNode(p2);
		 ndisc += 1;			/* Update number discarded. */
		}
	 else if( c1 > c2 )
		{DelTxNode(p1);
		 ndisc += 1;			/* Update number discarded. */
		}
	 else 
		fatal("cputomis: address error following #TYPE\n" );
	} /* END OF for(n = wordsize; n < tysize; n += wordsize) */
	
 p1 = GetTxNextNode(p);
 switch(GetTxOpCodeX(p1))
	{case G_PUSH:				/* G_PUSH src =>
						 *         G_ADD3 &tysz,%sp,%sp
						 *         G_MMOV src,-tysz(%sp) 
						 */
		p2 = MakeTxNodeBefore(p1,G_ADD3);
		PutTxOperandAd(p2,1,GetAdAddToKey(Tword,i0,tysize));
		PutTxOperandAd(p2,2,(ad1 = GetAdCPUReg(CSP)));
		PutTxOperandAd(p2,3,ad1);
		PutTxOpCodeX(p1,G_MMOV);
		PutTxOperandAd(p1,3,GetAdDispInc(Tword,"",CSP,-tysize));
		break;
	case G_MOV:				/* G_MOV => G_MMOV */
		PutTxOpCodeX(p1,G_MMOV);
		break;
	default:
		fatal("cputomis: illegal instruction following #TYPE\n" );
	}

					/* fixup the LSB info in src and dest */
 for(operand = 2; operand <= 3; operand++)
	{ad1 = GetTxOperandAd(p1,operand);
	 md1 = GetAdMode( ad1 );
	 switch( md1 )
		{case Absolute:
		 case Disp:
		 case IndexRegDisp:
			PutTxOperandAd(p1,operand,
				GetAdAddToKey(GetTxOperandType(p1,operand),ad1,
					tysize - wordsize));
		}
	} 
					/* fixup type info in src and dest */
 PutTxOperandType(p1,2,ty);
 PutTxOperandType(p1,3,ty);

 DelTxNode(p);				/* Delete #TYPE node        */
 ndisc += 1;				/* Update number discarded. */

 if(DBdebug(2,XINCONV))
	prafter(GetTxPrevNode(p1),GetTxNextNode(p1),0);

 return(p1);
}
	STATIC TN_Id
mistocpu(p)		/* Converts 
			 * 1) "G_MMOV 0,0,X,Y" to a sequence of G_MOV's
			 * 2) "G_ADD3 0,n,%sp,%sp" followed by "G_MMOV 0,0,X,-n(%sp)"
			 *  to a sequence of G_PUSH's.
			 * Do this only if no MAU register ops.
			 */
TN_Id p;

{AN_Id ad;
 TN_Id before;			/* node before G_MMOV */
 extern boolean chkmmov();
 extern AN_Id i0;
 extern void instructprint();
 int j;
 unsigned long lastlive[NVECTORS];
 extern unsigned ld_maxword;
 AN_Mode md;
 extern unsigned int ndisc;	/* Number of instructions discarded.	*/
 unsigned int operand;		/* Operand counter.	*/
 TN_Id pp;			/* For debugging */
 extern void prafter();
 extern void prbefore();
 TN_Id q;
 RegId ri;
 OperandType ty;
 OperandType ty3;		/* Type of third operand. */
 long int ty3size;		/* Size of third operand in bytes. */
 long int wordsize;		/* Size of a word in bytes. */

						/* Check operands.	*/
 ty3 = GetTxOperandType(p,3);			/* Get type of third operand. */
 if(!chkmmov(GetTxOperandType(p,2),
		GetAdMode(GetTxOperandAd(p,2)),
		ty3,
		GetAdMode(GetTxOperandAd(p,3))))
	return(p);

 if(DBdebug(2,XOUTCONV)){
	pp = p;		/* save this for later debugging */
	prbefore(GetTxPrevNode(p),GetTxNextNode(p),0,"cputomis");
 }

						/* Rewrite instruction.	*/
 ty3size = TySize(ty3);				/* Compute size of 3rd opd. */
 wordsize = TySize(Tword);			/* Need size of a word.	*/
 PutTxOpCodeX(p,G_MOV);
 for(operand = 2; operand <= 3; operand++)
	 {ad = GetTxOperandAd(p,operand);
	  md = GetAdMode(ad);
	  ty = GetTxOperandType(p,operand);
	  switch(md) 
		{case Absolute:
		 case Disp:
		 case IndexRegDisp:
			PutTxOperandAd(p,operand,GetAdAddToKey(ty,ad,
				wordsize - ty3size));
		} /* END OF switch(md) */
	  PutTxOperandType(p,operand,Tword);
	} /* END OF for(operand = 2; operand <= 3; operand++) */

					/* Optimize out stack increment */
 q = GetTxPrevNode(p);
 if((GetTxOpCodeX(q) == G_ADD3) && 
		(GetTxOperandAd(q,1) == GetAdAddToKey(Tword,i0,ty3size)) &&
		(GetTxOperandAd(q,2) == GetAdCPUReg(CSP)) &&
		(GetTxOperandAd(q,3) == GetAdCPUReg(CSP)) &&
		(GetTxOperandAd(p,3) == GetAdDispInc(Tword,"",CSP,-ty3size)))
	{
	 if(DBdebug(2,XOUTCONV)){
		instructprint(stdout,q,0);
		(void)fprintf(stdout,"%cDELETED\n",ComChar);
	 }
	 DelTxNode(q);			/* Delete the "G_ADD3" instruction. */
	 ndisc += 1;			/* Update number discarded. */
	 PutTxOpCodeX(p,G_PUSH);
	 PutTxOperandAd(p,3,i0);
	}

					/* Save old live/dead info */
 before = GetTxPrevNode(p);
 GetTxLive(p,lastlive,ld_maxword);
					/* Create additional instructions */
 for(j = wordsize; j < ty3size; j += wordsize)
	{q = MakeTxNodeAfter(p,GetTxOpCodeX(p));
	 for(operand = 2; operand <= 3; operand++)
		{ad = GetTxOperandAd(p,operand);
		 md = GetAdMode(ad);
		 switch(md)
			{case CPUReg:
				ri = GetAdRegA(ad);
				ri = GetNextRegId(ri);
				PutTxOperandAd(q,operand,GetAdCPUReg(ri));
				break;
			 case Immediate:
				break;
			 default:
				PutTxOperandAd(q,operand,
					GetAdAddToKey(GetTxOperandType(p,operand),
						ad,wordsize));
			} /* END OF switch(md) */
		} /* END OF for(operand = 2; operand <= 3; operand++) */
	 if(IsAdDisp(GetTxOperandAd(p,2)) && IsAdCPUReg(GetTxOperandAd(p,3))
			&& (GetAdRegA(GetTxOperandAd(p,2))
				== GetAdRegA(GetTxOperandAd(p,3))
			   ))
		(void) MoveTxNodeAfter(p,q);
	 p = q;
	} /* END OF for(j = wordsize; j < ty3size ; j += wordsize) */

				/* update live/dead info for new instr */
 updlive(before,GetTxNextNode(p),lastlive,ld_maxword);

 if(DBdebug(2,XOUTCONV))
	prafter(GetTxPrevNode(pp),GetTxNextNode(q),0);

 return(p);
}
	boolean
chkmmov(ty2,md2,ty3,md3)	/* Checks operands of MIS move.	*/
		 	/* Check operands of move to see if can be converted */
			/* to CPU move(s) */
OperandType ty2,ty3;
AN_Mode md2,md3;

{
 if(ty2 != ty3)
	return(FALSE);
 if((md2 == MAUReg) || ! ((ty2 == Tsingle) || (md2 == CPUReg) || 
		(md2 == Absolute) || (md2 == Disp) || (md2 == IndexRegDisp)))
	return(FALSE);
 if((md3 == MAUReg) || ! ((ty3 == Tsingle) || (md3 == CPUReg) || 
		(md3 == Absolute) || (md3 == Disp) || (md3 == IndexRegDisp)))
	return(FALSE);
 return(TRUE);
}
