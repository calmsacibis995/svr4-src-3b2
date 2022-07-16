/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)newoptim:m32/inline.c	1.16"

/************************************************************************/
/*				inline.c				*/
/*									*/
/*		This file contains the in-line function expansion 	*/
/*	optimization.  							*/
/*									*/
/************************************************************************/

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
#include	"ALNodeDefs.h"
#include	"FNodeTypes.h"
#include	"FNodeDefs.h"
#include	"optab.h"
#include	"optim.h"
#include	"optutil.h"
#include	"debug.h"

#define	DESTINATION	3
#define	MAXINSTR	50	/* max instructions for in-line candidates */
#define ILDEFAULT	20	/* default max pc in-line growth per file */
#define ILLEVEL		3	/* number of levels of in-line expansion */

	/* Private function declarations. */
STATIC boolean expandable();	/* check for complicated uses of labels */
STATIC boolean dealloc();	/* deallocate saved registers */
STATIC void ilcall();		/* expands a call recursively */
STATIC void ilcopynaqs();	/* copy NAQ info to new addr nodes */
STATIC void iledit();		/* edit to create a candidate */
STATIC void ilfloat();		/* remove floating point conversions */
STATIC void ilinsert();		/* insert code inline */
STATIC int ilnargs();		/* get number of arguments */
STATIC void ilpushmov();	/* changes pushes to moves */
STATIC TN_Id ilscanargs();	/* scan for arguments of function */
STATIC AN_Id isregarg();	/* check whether a register is an argument */

	void
ilinit()			/* initialization for in-line substitution */
{extern int intpc;		/* percent in-line limit; in GlobalDefs.c */
 extern int optmode;		/* optimization mode */

 /* default the in-line expansion limit if not set */
 if(intpc == -3)
	switch(optmode)
		{case OSPEED: intpc = -1; break;
		 case OSIZE: intpc = 0; break;
		 default: intpc = ILDEFAULT;
		}
}

	void
pcdecode(flags)			/* decode limit on percent in-line expansion */
char *flags; 			/* ptr to first char of suboption for 'y' */

{extern int intpc;		/* percent in-line limit; in GlobalDefs.c */
 char *p;			/* general char pointer */
 extern long strtol();		/* Converts ASCII to long; in C(3) library. */
 switch(*flags)
	{case 'u': intpc = -1; break;
	 case 's': intpc = -2; break;
	 case '\n':
	 case '\0': 
		fprintf(stderr,"Optimizer: in-line option missing, ");
		fprintf(stderr,"expansion suppressed\n");
		flags--;
		intpc = -2;
		break;
	 default:
		intpc = (int) strtol(p = flags,&flags,10);
		flags--;
		if(flags >= p) break; /* break if found an integer */
		fprintf(stderr,"Optimizer: invalid in-line option ");
		fprintf(stderr, "starting with '%c', ",*p);
		fprintf(stderr, "expansion suppressed\n");
		intpc = -2;
	}
 return;		/* returns pointer to last character of suboption */
}

	void
ilcandidate()		/* save candidate for inline expansion */

{ 
 extern FN_Id FuncId;		/* FN_Id of current func; in GlobalDefs.c */
 STATIC boolean expandable();	/* check for complicated uses of labels */
 STATIC boolean dealloc();	/* deallocate saved registers */
 extern void funcprint();	/* debug function print; in debug.c */
 STATIC void iledit();		/* edit to create a candidate */
 extern int intpc;		/* percent in-line limit; in GlobalDefs.c */
 extern unsigned int praddr();	/* Prints address nodes.	*/
 extern boolean swflag;		/* switch table in function; in local.c */

 if(intpc == -2)
	return;				/* check for in-line being suppressed */

 PutFnCandLocSz(FuncId,GetFnLocSz(FuncId));

 				/* check whether function is candidate */
				/* for in-line expansion*/
			
 if(!IsFnBlackBox(FuncId) &&	/* No black box in function */
		!swflag &&	/* No switch tables in function */
				/* In archive or otherwise small enough */
		(IsFnLibrary(FuncId) || GetFnInstructions(FuncId) <= MAXINSTR) &&
		expandable() &&	/* No complicated label expressions */
		dealloc() )	/* we can deallocate any saved registers  */
	{
	 iledit();			/* edit function to make candidate */
 	 if(DBdebug(3,XIL)) 
		{fprintf(stdout,"\n%ccandidate: ",ComChar);
		 (void)praddr(GetFnName(FuncId),Tbyte,stdout);
		 funcprint(stdout,"candidate: in_line expansion candidate",0);
		}
	 PutFnFunction(FuncId);		/* link text node and private nodes */
	 PutFnCandidate(FuncId,TRUE);	/* indicate it is an candidate */
	}
 else
	{PutFnCandLocSz(FuncId,0);
	}
 return;
}

	STATIC boolean
expandable()			/* Can we expand the function in question?
				 * Return TRUE if so, FALSE otherwise.
				 * Since ilinsert() must rewrite text labels, we
				 *  make sure that there aren't any complicated
				 *  references.
				 * Currently, we only catch operands of the form 
				 *  '*.Ln+m', where .Ln is some text label in the
				 *  function.
				 * We also look for nonstandard uses of %fp.
				 */
{
 extern unsigned int Max_Ops;	/* maximum operands in an instruction */
 AN_Id an_id;
 AN_Id anfp;		
 register unsigned int i;
 register unsigned int op;
 register TN_Id tn_id, tn_id2;

 anfp = GetAdCPUReg(CFP);
 for(ALLN(tn_id))
	{op = GetTxOpCodeX(tn_id);
	 if(IsOpPseudo(op) || IsOpAux(op))
		continue;
	 if(op == RESTORE || op == SAVE || op == HLABEL)
		continue;
	 if(op != LABEL)	/* look for uses of %fp */
		{for(i = 0; i < Max_Ops; ++i)
			{if(GetTxOperandAd(tn_id,i) == anfp)
				return FALSE;
			}
		 continue;
		}
		/* here if a non-hard label */
	 an_id = GetTxOperandAd(tn_id,0);
		/* look for references to this label */
	 for(ALLN(tn_id2))
	 	{for( i = 0; i < Max_Ops; i++ )
			{
			 if( GetTxOperandType(tn_id2,i) == Tnone ) break;
				/* simple labels are okay */
			 if(GetTxOperandAd(tn_id2,i) == an_id) continue;
				/* more compilcated uses of labels are not */
			 if(IsAdUses(GetTxOperandAd(tn_id2,i),an_id)) 
				return FALSE;
			}
		}
	 }
 return TRUE;
}
	STATIC boolean
dealloc()			/* deallocate saved registers */
				/* returns true if no saved registers left */

{extern FN_Id FuncId;		/* FN_Id of current function; in GlobalDefs.c*/
 extern unsigned int Max_Ops;	/* maximum oprands in an instruction */
 unsigned int R3; 		/* register 3 */
 unsigned int R8; 		/* register 8 */
 extern int TySize();		/* returns size in bytes of given type */
 register AN_Id an_id;		/* general address node id */
 AN_Mode an_mode;		/* general address mode */
 AN_Id disp[9];			/* displacement address id's */
 AN_Id dispdef[9];		/* displacement deferred address id's */
 char *expr;			/* expression part of address */
 extern void funcprint();	/* debug function print; in debug.c */
 extern void fatal();		/* exit with error diagnostic */
 register unsigned int i;	/* General integer.	*/
 STATIC AN_Id isregarg();	/* check whether a register is an argument */
 extern AN_Id i0;		/* Immediate 0; in GlobalDefs.c.	*/
 unsigned long int locsize;	/* Size of local area before deallocation. */
 long int na;			/* Number of arguments.	*/
 int nr;			/* Register number.	*/
 int ns;			/* Scratch register counter.	*/
 unsigned short op;
 unsigned int tszword;		/* Size of a (target machine's) word.	*/
 register TN_Id tn_id;		/* General text node id.	*/
 OperandType type;		/* Type of an operand.	*/
 boolean ur[3];			/* Scratch registers used.	*/

 if(GetFnNumReg(FuncId) == 0) return(TRUE);	/* no saved registers */

				/* check for available scratch registers */
				/* if needed for deallocating saved */
				/* registers used in displacement mode */
 R3 = GetRegNo(CREG3);
 for(ns = 0; ns < R3; ++ns)
	ur[ns] = FALSE;
 for(ALLNSKIP(tn_id))
	{for( i = 0; i < Max_Ops; i++ )
		{if(GetTxOperandType(tn_id,i) == Tnone) continue;
		 an_id = GetTxOperandAd(tn_id,i);
		 switch(an_mode = GetAdMode(an_id))
			{
			 case IndexRegDisp:
			 case IndexRegScaling:
		 		nr = GetRegNo(GetAdRegB(an_id));
				if( nr < R3 ) ur[nr] = TRUE;
				/* FALLTHRU */
			 case PreDecr:
			 case PreIncr:
			 case PostDecr:
			 case PostIncr:
			 case CPUReg:
			 case Disp:
			 case DispDef:
		 		nr = GetRegNo(GetAdRegA(an_id));
				if( nr < R3 )
					{ur[nr] = TRUE;
					 if(an_mode == CPUReg 
						&& GetTxOpCodeX(tn_id) == G_MMOV)
					    {switch(GetTxOperandType(tn_id,i))
						{case Tdblext:
						    if(nr+2 < R3)
							ur[nr+2] = TRUE;
						    /* FALLTHRU */
						 case Tdouble:
						    if(nr+1 < R3)
							ur[nr+1] = TRUE;
							endcase;
						}
					    }
					}
				endcase;
			 case Absolute:
			 case AbsDef:
			 case StatCont:
			 case Immediate:
			 case MAUReg:
				endcase;
			 default:
				fatal("dealloc: unknown mode (%d).\n",
					an_mode);
				endcase;
			}
		}
	}

 if(DBdebug(3,XIL)) 
	funcprint(stdout,"dealloc: in_line before deallocation",0);

				/* get address nodes. Allocate full word, */
				/* regardless of data size */
 na = 0;
 locsize = GetFnCandLocSz(FuncId);
 tszword = TySize(Tword);
					/* Loop through saved registers */
					/* User register space is R3-R8, allocated
					 * in reverse order */
 R8 = GetRegNo(CREG8);
 i = R8 - GetFnNumReg(FuncId);		/* R8 - (no of saved registers) */
 for(nr = R8; nr > i ; --nr)
	{
				/* get address of argument in arg area */
				/* if it is a register argument */
	 if((an_id = isregarg(GetRegId(nr))) == NULL)
				/* otherwise make space in local area */
				/* if it is a register local */
		{an_id = GetAdDispInc(Tword,"",CFP,
			(long int) (locsize + tszword * na));
		 PutAdCandidate(an_id,TRUE); /* this one for candidate use only */
		 na++;
		}
				/* in either case, get stack references to */
				/* subsititute for register references */
	 disp[nr] = an_id;
	 dispdef[nr] = GetAdAddIndInc(Taddress,Tword,an_id,0);
				/* create REGAL type info */
	 PutAdGnaqType(an_id,NAQ);
	 PutAdFP(an_id,MAYBE);
	 PutAdGNAQSize(an_id,tszword);
	}

				/* translate operands */
for(ALLN(tn_id))	  	/* loop through instructions */
	{			/* change registers saved and frame size */
	 op = GetTxOpCodeX(tn_id);
	 if(IsOpPseudo(op))			/* Don't touch pseudo-ops. */
						/* FBW: IS THIS OK?	*/
		continue;
	 if(op == SAVE || op == ISAVE)
		{if(op == SAVE) 
			PutTxOperandAd(tn_id,0,GetAdCPUReg(CFP));
		 else
			PutTxOperandAd(tn_id,0,i0);
		 tn_id = skipprof(tn_id);
		 if(GetTxOpCodeX(tn_id) == G_ADD3 
				&& GetTxOperandAd(tn_id,DESTINATION) ==
						GetAdCPUReg(CSP))
			{PutTxOperandAd(tn_id,1,
				GetAdAddToKey(Tword, i0,
					(long int) (locsize + tszword * na)));
			}
		 else fatal( "dealloc:no frame size increment\n", NULL );
		 continue;
		}

	ns = 0;
	for(i = 0; i < Max_Ops; i++)	/* Loop through operands.	*/
		{if((type = GetTxOperandType(tn_id,i)) == Tnone)
			continue;
		 an_id = GetTxOperandAd(tn_id,i);
		 switch(an_mode = GetAdMode(an_id))
			{case IndexRegDisp:
			 case IndexRegScaling:
		 		nr = GetRegNo(GetAdRegB(an_id));
				if(R3 <= nr && nr <= R8) return(FALSE);
				/* FALLTHRU */
			 case PreDecr:
			 case PreIncr:
			 case PostDecr:
			 case PostIncr:
		 		nr = GetRegNo(GetAdRegA(an_id));
				if(R3 <= nr && nr <= R8) return(FALSE);
				endcase;
			 case CPUReg:
		 		nr = GetRegNo(GetAdRegA(an_id));
				if(nr < R3 || R8 < nr) endcase;
				PutTxOperandAd(tn_id,i,disp[nr]);
				endcase;
			 case Disp:
		 		nr = GetRegNo(GetAdRegA(an_id));
				if(nr < R3 || R8 < nr) endcase;
				/**************************************************/
				/* rewrite 					  */
				/*	op     ...0(%rm)...			  */
				/*		||				  */
				/*		vv				  */
				/*	op     ...*n(%fp/%ap)...		  */
				/*	(provided that 0(%rm) isn't an operand	  */
				/*	 of a G_MMOV instruction because this 	  */
				/*	 rewrite would make it impossible to 	  */
				/*	 unravel a #TYPE DOUBLE for example)	  */
				/* and						  */
				/* 	op     ...i(%rm)...   			  */
				/*		||				  */
				/*		vv				  */
				/*	G_MOV  n(%fp/%ap),%rj  (rj unused scratch)*/
				/*	op     ...i(%rj)...			  */
				/**************************************************/
				if(IsAdNumber(an_id) 
					&& (GetAdNumber(type,an_id)) == 0
					&& GetTxOpCodeX(tn_id) != G_MMOV)
					{PutTxOperandAd(tn_id,i,dispdef[nr]);
					 endcase;
					}
					/* find the next available scratch */
				while(ns < R3 && ur[ns]) ns++;
				if(ns == R3) /* scratch reg not avail */
					return(FALSE);
				tn_id = MakeTxNodeBefore(tn_id,G_MOV);
				PutTxOperandAd(tn_id,2,disp[nr]);
				PutTxOperandType(tn_id,2,Tword);
				PutTxOperandAd(tn_id,DESTINATION,
					GetAdCPUReg(GetRegId(ns)));
				PutTxOperandType(tn_id,DESTINATION,Tword);
				tn_id = GetTxNextNode(tn_id);
				expr = GetAdExpression(type,an_id);
				PutTxOperandAd(tn_id,i,
					GetAdDisp(type,expr,GetRegId(ns)));
				ns++;
				endcase;
			 case DispDef:
		 		nr = GetRegNo(GetAdRegA(an_id));
				if(nr < R3 || R8 < nr) endcase;
				/**************************************************/
				/* rewrite 					  */
				/* 	op     ...*i(%rm)...    		  */
				/*		||				  */
				/*		vv				  */
				/*	G_MOV  n(%fp/%ap),%rj  (rj unused scratch)*/
				/*	op     ...*i(%rj)...			  */
				/**************************************************/
					/* find the next available scratch */
				while(ns < R3 && ur[ns]) ns++;
				if(ns == R3) /* scratch reg not avail */
					return(FALSE);
				tn_id = MakeTxNodeBefore(tn_id,G_MOV);
				PutTxOperandAd(tn_id,2,disp[nr]);
				PutTxOperandType(tn_id,2,Tword);
				PutTxOperandAd(tn_id,3,GetAdCPUReg(GetRegId(ns)));
				PutTxOperandType(tn_id,3,Tword);
				tn_id = GetTxNextNode(tn_id);
				expr = GetAdExpression(type,an_id);
				PutTxOperandAd(tn_id,i,GetAdDispDef(expr,GetRegId(ns)));
				ns++;
				endcase;
			 case Absolute:
			 case AbsDef:
			 case StatCont:
			 case Immediate:
			 case MAUReg:
				endcase;
			 default:
				fatal("dealloc: unknown mode (%d).\n", 
					an_mode);
				endcase;
			}
		}
	}

	if(DBdebug(3,XIL))
		funcprint(stdout,"dealloc: in-line after deallocation",0);
					/* store new values */
	PutFnCandLocSz(FuncId,locsize + na * tszword);
	return(TRUE);
}
	STATIC AN_Id
isregarg(regid)		/* checks whether the register could be a reg arg */
RegId regid;
		/* This test could pick up a local in certain cases, but only
		where the local is acting like a reg arg and can be treated 
		as such. */

{
 extern unsigned int Max_Ops;	/* maximum operands in an instruction */
 AN_Id adset;			/* address id of argument */
 register AN_Id an_id;		/* general address node id*/
 register unsigned int i;	/* general integer */
 unsigned int op;		/* opcode of a text node */
 AN_Id regAP;			/* an_id of %ap */		
 AN_Id regan_id;		/* register address node id */
 TN_Id tnset;			/* id of text node that initializes arg reg */
 register TN_Id tn_id;		/* general text node id */

 regan_id = GetAdCPUReg(regid);
 adset = NULL;
 for(ALLNSKIP(tn_id))
	{			 /* look for move from arg to reg */
	 op = GetTxOpCodeX(tn_id);
	 if(IsOpAux(op))
		continue;
	 if(op == G_MOV)
		{
		 an_id = GetTxOperandAd(tn_id,2);
		 if(IsAdDisp(an_id) && GetAdRegA(an_id) == CAP
			&& GetTxOperandAd(tn_id,DESTINATION) == regan_id)
			{adset = an_id;
			 tnset = tn_id;
	     	  	 break;
			}
		}
				/* check for other use of register */
				/* before it is set from arg */
	 for( i = 0; i < Max_Ops; i++ )
		if(IsAdUses(GetTxOperandAd(tn_id,i),regan_id))
			return((AN_Id) NULL);
				/* check for end of first basic block */
	 if(IsTxLabel(tn_id) || IsTxBr(tn_id)) return((AN_Id) NULL);
	}
 if(adset == NULL) return((AN_Id) NULL);

 regAP = GetAdCPUReg(CAP);
 for(ALLN(tn_id))
	{if(tn_id == tnset ) continue;
				/* look for other use of arg */
				/* or taking address of args */
	for( i = 0; i < Max_Ops; i++ )
		{if(GetTxOperandType(tn_id,i) == Tnone) continue;
		 if(IsAdUses(GetTxOperandAd(tn_id,i),adset)) 
			return((AN_Id) NULL);
		 if(GetTxOperandAd(tn_id,i) == regAP)
			return((AN_Id) NULL);
		}
	switch(GetTxOpCodeX(tn_id))
		{case G_MOVA:
		 case G_PUSHA:
			if(IsAdDisp(GetTxOperandAd(tn_id,2))
				&& GetAdRegA(GetTxOperandAd(tn_id,2)) == CAP)
				return((AN_Id) NULL);
		}
	}
return(adset);
}

	STATIC void
iledit()			/* edit routine to make a candidate */

{extern FN_Id FuncId;		/* FN_Id of current func; in GlobalDefs.c */
 AN_Id an_id;			/* general address node id */
 extern void fatal();		/* exit with error diagnostic */
 register TN_Id labtn;		/* text node id of label inserted at eof */
 register boolean labused;	/* true if label inserted at end is used */
 extern unsigned int ndisc;	/* Number of instructions discarded.	*/
 register TN_Id tn_id;		/* general text node id */

				/* replace last return with lab */
				/* or follow last jump with lab */
 for(tn_id = GetTxPrevNode((TN_Id) NULL); 
		tn_id; 
		tn_id = GetTxPrevNode(tn_id))
	{if(IsTxRet(tn_id) || IsTxUncBr(tn_id))
		{if(GetTxOpCodeX(tn_id) == RET) 
			{DelTxNode(GetTxPrevNode(tn_id));
			 ndisc += 1;		/* Update number discarded. */
			}
		 labtn = tn_id;
		 PutTxOpCodeX(tn_id,LABEL);
		 PutTxOperandType(tn_id,0,Tbyte);
		 an_id = GetAdAbsolute(Tbyte,":RL:");
		 PutAdLabel(an_id,TRUE);	/* It is a label.	*/
		 PutTxOperandAd(tn_id,0,an_id);
		 while(GetTxNextNode(tn_id))
			{DelTxNode(GetTxNextNode(tn_id));
			 ndisc += 1;
			}
		 break;
		}
	}

 labused = FALSE;
 for(ALLN(tn_id))
	{if(GetTxOpCodeX(tn_id) == SAVE || GetTxOpCodeX(tn_id) == ISAVE)
		{		/* find stack increment */
		 tn_id = skipprof(tn_id);
		 an_id = GetTxOperandAd(tn_id,1);
		 if(GetTxOpCodeX(tn_id) == G_ADD3 
				&& GetTxOperandAd(tn_id,DESTINATION) ==
						GetAdCPUReg(CSP)
				&& IsAdImmediate(an_id)
				&& IsAdNumber(an_id))
			PutFnCandLocSz(FuncId,
				(unsigned long)
					GetAdNumber(GetTxOperandType(tn_id,1),
						an_id));
		 else
			fatal( "iledit: frame increment missing\n");
				/* delete up to and including */
				/* stack size increment */
		 tn_id = GetTxNextNode(tn_id);
		 while(GetTxPrevNode(tn_id)) 
			{DelTxNode(GetTxPrevNode(tn_id));
			 ndisc += 1;		/* Update number discarded. */
			}
		}
				/* delete hard labels */
	 if(GetTxOpCodeX(GetTxNextNode(tn_id)) == HLABEL) 
		{DelTxNode(GetTxNextNode(tn_id));
		 ndisc += 1;			/* Update number discarded. */
		}
				/* replace other returns with jump */
				/* to new label */
	 if(IsTxRet(tn_id))
		{if(GetTxOpCodeX(tn_id) == RET) 
			{DelTxNode(GetTxPrevNode(tn_id));
			 ndisc += 1;		/* Update number discarded. */
			}
		 PutTxOpCodeX(tn_id,IJMP);
		 PutTxOperandType(tn_id,0,Tbyte);
		 PutTxOperandAd(tn_id,0,GetAdAbsolute(Tbyte,":RL:"));
		 labused = TRUE;
		}
	}
					/* if no jumps are taken to the */
					/* last label -- remove it */
 if(!labused)
	{DelTxNode(labtn);
	 ndisc += 1;				/* Update number discarded. */
	}
}

	int
iliscalled(name) 			/* Checking whether a library */
					/* function is called and not defined*/
char *name; 				/* library function name */

{
 extern void fatal();			/* exit with error diagnostic */
 FN_Id fn_id;				/* id of the function*/

 if((fn_id = GetFnIdProbe(GetAdAbsolute(Tbyte,name))) == NULL)
	return(0);
 if(IsFnDefined(fn_id))
	fatal("iliscalled: func in src library already defined (%s)\n",name);
 return((GetFnCalls(fn_id) == 0) ? 0 : 1 );
}
	 void
ildecide()			 /* Deciding which candidates to expand */

{
 int delta;			/* percent growth per call scaled up by 100 */
 extern void fnprint();		/* prints function nodes */
 register FN_Id fn_id,fn_id2;	/* general function id's */
 int fpc;			/* percent limit scaled up by 100 */
 extern int intpc;		/* percent limit on in-line; in GlobalDefs.c */
 long int totalni = 0;		/* total number of instructions in file */

				/* check whether in-line expansion */
				/* is being suppressed */
 if(intpc == -2)
	return;

 if(DBdebug(0,XIL))
	fnprint("ildecide: before selecting candidates");

						/* Scan function nodes. */
 for(fn_id = GetFnNextNode((FN_Id) NULL); fn_id; fn_id = fn_id2)
	{fn_id2 = GetFnNextNode(fn_id);
				/* count total number of instructions */
	 if(!IsFnLibrary(fn_id)) 
		totalni += GetFnInstructions(fn_id);
				/* eliminate uncalled functions from */
				/* candidate list */
	 if(GetFnCalls(fn_id) == 0)
		PutFnCandidate(fn_id,FALSE);
				/* Eliminate nodes for funcs not defined */
	 if(!IsFnDefined(fn_id))
		DelFnNode(fn_id);
	}
						/* Sort the functions by size */
					/* whether we are going to constrain */
					/* size or not */
 SortFnNodes();

 if(intpc != -1)		/* apply percent limit unless no limit */
	{			/* sort in increasing order of size */
	 fpc = 100 * intpc;	/* apply percent text increase constraint */
	 for(fn_id = GetFnNextNode((FN_Id) NULL);
			fn_id;
			fn_id = GetFnNextNode(fn_id))
		{if(!IsFnCandidate(fn_id)) continue;
		 PutFnExpansionLimit(fn_id,GetFnCalls(fn_id));
		 delta = 100 * (GetFnInstructions(fn_id) - 2) * 100 / totalni;
		 fpc -= GetFnExpansionLimit(fn_id) * delta;
		 if(fpc < 0)
			{while(fpc < 0 && GetFnExpansionLimit(fn_id) > 0)
				{fpc += delta;
				 PutFnExpansionLimit(fn_id,
					GetFnExpansionLimit(fn_id) - 1);
				}
			 break;
			}
		}
					/* set calls to 0 for unused funcs */
	 if(fn_id) 
		{if(GetFnExpansionLimit(fn_id) == 0) 
			PutFnCandidate(fn_id,FALSE);
	 	 while((fn_id = GetFnNextNode(fn_id)) != NULL)
		 	PutFnCandidate(fn_id,FALSE);
		}
	}
 if(DBdebug(0,XIL))
	fnprint("ildecide: after selecting candidates");
 return;
}

	void
ilfunc()			/* perform the in-line substitution */
				/* for function */

{
 extern void DelFrames();	/* delete frames from stack */
 extern FN_Id FuncId;		/* FN_Id of current function; in GlobalDefs.c*/
 extern void addrprint();	/* debug address table print; in debug.c */
 extern void fatal();		/* exit with diagnostic; in fatal.c */
 extern void fnprint();		/* prints function nodes */
 STATIC void ilcall();		/* expands a call recursively */
 extern int intpc;		/* percent limit on in-line; in GlobalDefs.c */
 extern AN_Id i0;		/* immediate 0; in GlocalDefs.c */
 register TN_Id tn_id;		/* general text node id */

				/* check whether in-line expansion 
				/* is being suppressed */
 if(Xskip(XIL))                                 /* Optimize for this caller? */
        return;                                 /* NO.  */
 if( intpc == -2 ) return;

				/* recursively in-line expand calls */
 ilcall((TN_Id) NULL,(TN_Id) NULL,1);

				/* free the frame list */
 DelFrames();

 				/* fix up caller's frame size */
				/* to include all callee's args and locals */ 
 tn_id = skipprof(GetTxNextNode((TN_Id) NULL));
 if(GetTxOpCodeX(tn_id) == G_ADD3 
	&& GetTxOperandAd(tn_id,DESTINATION) == GetAdCPUReg(CSP))
	PutTxOperandAd(tn_id,1,
		GetAdAddToKey(Tword,i0,(long int) GetFnLocSz(FuncId)));
 else fatal( "ilfunc: frame increment missing\n");
				/* print address table */
 if(DBdebug(0,XIL)) fnprint("ilfunc: after in-line expansion");
 if(DBdebug(3,XIL)) addrprint("ilfunc: after in-line expansion");

 return;
}

	STATIC void
ilcall(tn_id1,tn_id2,illevel)	/* recursively look for calls to expand */
TN_Id tn_id1;			/* node preceding code to search */
TN_Id tn_id2;			/* node following code to search */
int illevel;			/* in-line expansion level */

{
 extern unsigned long AssignFrame();/* assign offset for a frame; in Frame.c */
 extern FN_Id FuncId;		/* FN_Id of current function; in GlobalDefs.c*/
 extern boolean PopParent();	/* pop entry from parent stack; in Frame.c */
 extern void PushParent();	/* push entry on parent stack; in Frame.c */
 extern int TySize();		/* returns size in bytes of given type */
 TN_Id argbefore;		/* id of node before first arg increment */
 extern void fnprint();		/* prints functions nodes */
 FN_Id fn_id;			/* general function node id */
 extern void funcprint();	/* debug function print; in debug.c */
 STATIC void ilcopynaqs();	/* copy NAQ info to new addr nodes */
 STATIC void ilfloat();		/* remove floating point conversions */
 STATIC void ilinsert();	/* insert code inline */
 STATIC int ilnargs();		/* get number of arguments */
 STATIC void ilpushmov();	/* changes pushes to moves */
 STATIC TN_Id ilscanargs();	/* scan for arguments of function */
 extern int intpc;		/* percent limit on in-line; in GlobalDefs.c */
 int nargs;			/* number of arguments for function */
 unsigned long int newlocsize;	/* trial new locsize */
 register unsigned long int offset; /* offset of function frame */
 extern void prafter();		/* debug before window print; in debug.c */
 extern void prbefore();	/* debug before window print; in debug.c */
 TN_Id tnf, tnl;		/* nodes before and after call */
 register TN_Id tn_id;		/* general address node id */

				/* Loop backward though function looking */
				/* for CALL.  Backward scan is used to */
				/* facilitate expansion of nested calls */
 for(tn_id = GetTxPrevNode(tn_id2); tn_id != tn_id1;
	tn_id = GetTxPrevNode(tn_id))
	{			/* consider function calls */
	 if(!(GetTxOpCodeX(tn_id) == ICALL || GetTxOpCodeX(tn_id) == CALL))
		continue;
				/* if function is not in candidate list, */
				/* there is nothing to expand */
	 fn_id = GetFnIdProbe(GetTxOperandAd(tn_id,1));
	 if(fn_id == NULL
			|| !IsFnDefined(fn_id)
			|| !IsFnCandidate(fn_id))
		continue;
				/* if there is a size limit, check */
				/* whether we have already done allowed */
				/* number of expansions for this function */
	 if(intpc != -1 && GetFnExpansions(fn_id) 
		>= GetFnExpansionLimit(fn_id)) continue;
				/* get number of arguments and scan for */
				/* arguments, flagging stack increments, */
				/* and give up if finding the arguments */
				/* is too hard */
	 if((nargs = ilnargs(tn_id)) < 0) continue;
	 offset = AssignFrame(fn_id,nargs);
 	 if((argbefore = ilscanargs(tn_id,nargs,offset,TRUE)) == NULL)
		continue;
				/* assign a frame offset and */
			 	/* change pushes to moves */
	 if(DBdebug(1,XIL_PUSH))
		funcprint(stdout,"ilcall: in-line before push-to-move",0);

	 ilpushmov(tn_id,argbefore,offset);  

	 if(DBdebug(2,XIL_PUSH))
		funcprint(stdout,"ilcall: in-line after push-to-move",0);
	 				/* insert code in-line */
	 tnf = GetTxPrevNode(tn_id);
	 tnl = GetTxNextNode(tn_id);
	 if(DBdebug(1,XIL_INSERT))
		funcprint(stdout,"ilcall: in-line before insert",0);
	 if(DBdebug(0,(XIL|XIL_INSERT)))
		prbefore(tnf,tnl,0,"in-line insert");

	 ilinsert(fn_id,tn_id,nargs,offset);

	 if(DBdebug(0,(XIL|XIL_INSERT)))
	 	prafter(tnf,tnl,0);
	 if(DBdebug(2,XIL_INSERT))
		funcprint(stdout,"ilcall: in-line after insert",0);

					/* copy NAQ info to new addr nodes */
	 ilcopynaqs(fn_id,nargs,offset);

	 				/* remove arg and ret value conv */
					/* for floats */
	 if(DBdebug(1,XIL_FP))
		funcprint(stdout,"ilcall: in-line before fp arg/ret conv",0);

	 ilfloat(tnf,tnl,nargs,offset);

	 if(DBdebug(2,XIL_FP))
		funcprint(stdout,"ilcall: in-line after fp arg/ret conv",0);
					/* compute new frame size */
	 newlocsize = offset + (nargs * TySize(Tword)) + GetFnCandLocSz(fn_id);
	 if(newlocsize > GetFnLocSz(FuncId)) 
		PutFnLocSz(FuncId,newlocsize);
					/* incr expansions in func node */
	 PutFnExpansions(fn_id,GetFnExpansions(fn_id) + 1);
	 				/* expand recursively if unlimited */
					/* growth */
	 if(DBdebug(0,XIL)) fnprint("after in-line expansion of a function");
	 if(intpc == -1 && illevel < ILLEVEL)
		{PushParent(FALSE,offset);
		 ilcall(tnf,tnl,illevel + 1);
					/* remove all nest entries and normal */
					/* parent entry */
		 while(PopParent());
		}
	 tn_id = GetTxNextNode(tnf);	/* continue scan */
	} /* end of scan over text nodes */
 return;
} /* end of this function */

	STATIC int
ilnargs(tn_id)			/* get number of arguments */
				/* and return negative value if cannot do it */
register TN_Id tn_id;		/* text node id of call instruction */

{extern int TySize();		/* return size in bytes of given type */
 AN_Id an_id;			/* general address node id */
 int i;				/* general integer */
 int nargs;			/* number of arguments */

						/* Make sure its a call. */
 if(GetTxOpCodeX(tn_id) != ICALL && GetTxOpCodeX(tn_id) != CALL)
	return(-1);
						/* Get number.	*/
 an_id = GetTxOperandAd(tn_id,0);
 if(!IsAdNumber(an_id))
	return(-1);
 i = GetAdNumber(GetTxOperandType(tn_id,0),an_id);
 if(GetTxOpCodeX(tn_id) == ICALL)
	nargs = i;
 if(GetTxOpCodeX(tn_id) == CALL)
	nargs = (i - TySize(Tword) + 1) / -TySize(Tword);
 return(nargs);
}

	STATIC TN_Id
ilscanargs(calltn,nargs,offset,mark) /* recursively scan to beginning of args*/
TN_Id calltn;			/* text node id of call */
int nargs;			/* number of function arguments */
unsigned long int offset;	/* offset of fuction frame */
boolean mark;			/* true if stack increments should be marked */

{
 extern void PushParent();	/* push entry on parent stack; in Frame.c */
 extern int TySize();		/* returns size in bytes of given type */
 register int argcnt = 0;	/* argument count.	*/
 STATIC int ilnargs();		/* get number of arguments */
 int nest_nargs;		/* Number of arguments for nested call. */
 register TN_Id tn_id;		/* general text node id */
 TN_Id tn_id2;			/* text node of move in two-instruct "push" */

				/* search back the number of args  */
				/* marking text nodes that increment */
				/* the stack pointer for saving arguments */
				/* if scanning the arguments of the function */
				/* being expanded */
				/* -- error if we find the start of function */
				/* or a branch */
 for(tn_id = GetTxPrevNode(calltn); tn_id; tn_id = GetTxPrevNode(tn_id))
	{PutTxSPI(tn_id,FALSE);
				/* check for termination */
	 if(argcnt >= nargs || IsTxBr(tn_id) || IsTxLabel(tn_id)) break;
	 switch(GetTxOpCodeX(tn_id))
		{case G_PUSH:
		 case G_PUSHA:
			argcnt++;
				/* mark if scanning args of func being */
				/* expanded */
			if(mark) PutTxSPI(tn_id,TRUE);
			endcase;
		 case G_ADD3:
			if(GetTxOperandAd(tn_id,DESTINATION) !=
					GetAdCPUReg(CSP))
				break;
			tn_id2 = GetTxNextNode(tn_id);
			if(GetTxOperandAd(tn_id,2) == GetAdCPUReg(CSP) 
				    && IsAdImmediate(GetTxOperandAd(tn_id,1))
				    && IsAdNumber(GetTxOperandAd(tn_id,1))
				    && GetTxOpCodeX(tn_id2) == G_MMOV
				    && IsAdDisp(GetTxOperandAd(tn_id2,
						DESTINATION))
				    && GetAdRegA(GetTxOperandAd(tn_id2,
						DESTINATION))
						== CSP)
				{argcnt +=
					GetAdNumber(GetTxOperandType(tn_id,1),
						GetTxOperandAd(tn_id,1))
					/ TySize(Tword);
					/* mark if scanning arguments of */
					/* function being expanded */
				 if(mark) PutTxSPI(tn_id,TRUE);
				}
			else return((TN_Id) NULL); /* arg push too complicated*/
			endcase;
		 case CALL:	/* nested calls */
		 case ICALL:
				/* if scanning function being expanded, */
				/* push an entry on the parent stack */
				/* because the nested call cannot re-use */
				/* the frame in which it is nested */
			if(mark) PushParent(TRUE,offset);
				/* scan arguments, but do not mark them */
			if((nest_nargs = ilnargs(tn_id)) < 0) return((TN_Id) NULL);
			if((tn_id = ilscanargs(tn_id,nest_nargs,offset,FALSE)) 
				== NULL) 
				return((TN_Id) NULL); /* couldn't pass call */
			tn_id = GetTxNextNode(tn_id);
			endcase;
		 case G_SUB3:
			if(GetTxOperandAd(tn_id,DESTINATION) ==
					GetAdCPUReg(CSP))
				return((TN_Id) NULL); /* removing arguments */
			endcase;
		 case G_POP:
			return((TN_Id) NULL);	/* removing arguments */
		}
	}
 if(argcnt != nargs) return((TN_Id) NULL);
 return(tn_id);
}

	STATIC void
ilpushmov(calltn,argbefore,offset) /* change pushes to moves */
register TN_Id calltn;		/* id of text node containing the call */
TN_Id argbefore;		/* id of text node before first arg increment */
unsigned long int offset;	/* offset of frame for function call */

{
 extern int TySize();		/* returns size in bytes of given type */
 extern unsigned int ndisc;	/* Number of instructions discarded.	*/
 extern void prafter();		/* debug before window print; in debug.c */
 extern void prbefore();	/* debug before window print; in debug.c */
 register TN_Id tn_id;		/* general text node id */
 OperandType type;		/* Type of an operand.	*/

				/* go forward changing them */
				/* till back to original call */
 if(DBdebug(0,(XIL|XIL_PUSH)))
	prbefore(argbefore,calltn,0,"in-line expansion push-to-move" );
				/* rewrite the pushes */
 for(tn_id = GetTxNextNode(argbefore);
		tn_id != calltn; 
		tn_id = GetTxNextNode(tn_id))
	{if(!IsTxSPI(tn_id))
		continue;
	 switch (GetTxOpCodeX(tn_id))
		{case G_PUSH:
			PutTxOpCodeX(tn_id,G_MOV);
			endcase;
		 case G_PUSHA:
			PutTxOpCodeX(tn_id,G_MOVA);
			endcase;
		 case G_ADD3:
			tn_id = GetTxNextNode(tn_id);
			DelTxNode(GetTxPrevNode(tn_id));
			ndisc += 1;		/* Update number discarded. */
			endcase;
		} /* END OF switch(GetTxOpCodeX(tn_id) */
	 type = GetTxOperandType(tn_id,DESTINATION);
	 PutTxOperandAd(tn_id,DESTINATION,GetAdDispInc(type,"",CFP,
		(long int) offset));
	 offset += TySize(type);
	}

 if(DBdebug(0,(XIL|XIL_PUSH))) prafter(argbefore,calltn,0);

 return;
}

	STATIC void
ilinsert(fn_id,calltn,nargs,offset)	/*Insert a procedure.	*/
FN_Id fn_id;			/* Id of function to be inserted.	*/
TN_Id calltn;			/* Text node id of call to function.	*/
int nargs;			/* Number of arguments for function.	*/
unsigned long int offset;	/* Offset of frame for function.	*/

{extern boolean IsAdPrivate();	/* TRUE if node is a private address.	*/
 extern FN_Id FuncId;		/* Id of current function (caller).	*/
 extern unsigned int Max_Ops;	/* Maximum oprands in an instruction.	*/
 extern int TySize();		/* Returns size in bytes of given type.	*/
 TN_Id aftertn;			/* Id of node after inserted function.	*/
 AN_Id an_id;			/* General address node id.	*/
 AN_Mode an_mode;		/* General address mode.	*/
 TN_Id beforetn;		/* Id of text node before call.	*/
 char *expr;			/* Expression part of address.	*/
 extern void fatal();		/* Exit with diagnostic; in fatal.c.	*/
 char lab[10];			/* Label string buffer.	*/
 boolean labs;			/* TRUE if label used.	*/
 unsigned long int locoff;	/* Offset of the local area for the func */
 extern unsigned int ndisc;	/* Number of instructions discarded.	*/
 AN_Id newan;			/* Address node for new label.	*/
 extern void newlab();		/* Create new label.	*/
 TN_Id newtn;			/* Id of new instruction in caller.	*/
 AN_Id oldan;			/* Address nodes for old and new labels. */
 unsigned int op;		/* Opcode of a text node */
 register unsigned int operand;	/* Operand counter.	*/
 register TN_Id tn_id, tn_id2;	/* General text node id's.	*/
 OperandType type;		/* Type of an operand.	*/

					/* Delete the call node.	*/
 beforetn = GetTxPrevNode(calltn);
 DelTxNode(GetTxNextNode(beforetn));
 ndisc += 1;				/* Update number discarded. */

					/* Insert candidate's instructions, */
					/* rewriting args and locals *
					/* and omitting save */
 newtn = beforetn;
 locoff = offset + nargs * TySize(Tword);
 for(tn_id = GetFnFirst(fn_id); tn_id; tn_id = GetTxNextNode(tn_id))
	{					/* make new node */
	 op = GetTxOpCodeX(tn_id);
	 newtn = MakeTxNodeAfter(newtn,op);
	 for(operand = 0; operand < Max_Ops; operand++)	/*Loop though operands.*/
		{type = GetTxOperandType(tn_id,operand);
		 PutTxOperandType(newtn,operand,type);
		 if(IsTxOperandVol(tn_id,operand))	/* copy volatility. */
			PutTxOperandVol(newtn,operand,TRUE);
		 an_id = GetTxOperandAd(tn_id,operand);
					/* Copy non-Private operands. */
		 if(IsOpAux(op) || !IsAdPrivate(an_id))
			{PutTxOperandAd(newtn,operand,an_id);
			 continue;
			}
					/* Translate Private operands.	*/
		 switch(an_mode = GetAdMode(an_id))
			{case Disp:
				if(GetAdRegA(an_id) == CAP) /* argument */
					{expr = GetAdExpression(type,an_id);
					 an_id = GetAdDispInc(type,expr,CFP,
						(long int) (offset));
					 PutTxOperandAd(newtn,operand,an_id);
					}
				else if(GetAdRegA(an_id) == CFP) /* local */
					{an_id = GetAdAddToKey(type,an_id,
						(long int) (locoff));
					 PutTxOperandAd(newtn,operand,an_id);
					}
				else
					fatal("ilinsert:trans not supported\n");
					/*NOTREACHED*/
				endcase;
			 case DispDef:
				if(GetAdRegA(an_id) == CAP) /* *argument */
					{expr = GetAdExpression(type,an_id);
					 an_id = GetAdDispInc(Taddress,expr,CFP,
						(long int) (offset));
					 PutTxOperandAd(newtn,operand,
					 	GetAdAddIndInc(Taddress,type,
							an_id,0));
					}
				else if(GetAdRegA(an_id) == CFP) /* *local */
					{an_id = GetAdAddToKey(Tword,an_id,
						(long int) (locoff));
					 PutTxOperandAd(newtn,operand,an_id);
					}
				else
					fatal("ilinsert:trans not supported\n");
					/*NOTREACHED*/
				endcase;
			 case Absolute:
			 case AbsDef:
			 case CPUReg:
			 case StatCont:
			 case Immediate:
			 case MAUReg:
			 case IndexRegDisp:
			 case IndexRegScaling:
			 case PreDecr:
			 case PreIncr:
			 case PostDecr:
			 case PostIncr:
				fatal("ilinsert: trans not supported (%d)\n",
					an_mode);
			 default:
				fatal("ilcall: unknown mode (%d).\n", ilcall);
				endcase;
			} /* END OF switch(an_mode = GetAdMode(an_id)) */
		} /* END OF for(operand = 0; operand < MaxOps; operand++) */
	} /* END OF for(tn_id = GetFnFirst(fn_id); tn_id; tn_id = GetTx... */

 aftertn = GetTxNextNode(newtn);		/* The end of the expanded routine */

				/* rewrite labels */
 for(tn_id = GetTxNextNode(beforetn); tn_id != aftertn; 
		tn_id = GetTxNextNode(tn_id))
				/* consider only labels */
	{if(!IsTxLabel(tn_id)) continue;
				/* rewrite in .In format */
	 oldan = GetTxOperandAd(tn_id,0);
	 newlab(lab,".I",sizeof(lab));
	 newan = GetAdAbsolute(Tbyte,lab);	/* Make a new label.	*/
	 PutAdLabel(newan,TRUE);		/* Mark it a label.	*/
	 PutTxOperandAd(tn_id,0,newan);
				/* check for whether label is used */
				/* use and force label to look used */
				/* if it is there for LICM */
	 if(GetTxOpCodeX(GetTxPrevNode(tn_id)) == LOOP
			&& GetTxLoopType(GetTxPrevNode(tn_id)) == Condition)
		labs = TRUE;
	 else
		labs = FALSE;
	 for(tn_id2 = GetTxNextNode(beforetn); tn_id2 != aftertn;
			tn_id2 = GetTxNextNode(tn_id2))
		if(tn_id2 != tn_id && GetP(tn_id2) == oldan)
			{PutP(tn_id2,newan);
			 labs = TRUE;
			}
	
	 if (!labs) /* didn't use label and it is not a LOOP LCOND */
		{tn_id = GetTxPrevNode(tn_id);
	 	 DelTxNode(GetTxNextNode(tn_id));
		 ndisc += 1;			/* Update number discarded. */
		}
	}

 if(IsFnMISconvert(fn_id))			/* Inherit #TYPE attribute. */
	PutFnMISconvert(FuncId, TRUE);
}

	STATIC void
ilcopynaqs(fn_id,nargs,offset)	/* coy NAQ info to new addr nodes */
FN_Id fn_id;			/* function being expanded */
int nargs;			/* number of words of arguments */
unsigned long int offset;	/* frame offset of function */

{extern FN_Id FuncId;		/* function in which candidate being inserted */
 extern int TySize();		/* returns size in bytes of given type */
 AN_Mode an_mode;		/* mode of private node */
 char *expr;			/* expression part of address */
 extern void fatal();		/* Handles fatal errors.	*/
 AN_Id newan;			/* address node for callee */
 AN_Id oldan;			/* address id of private node of candidate */
 RegId regid;			/* register of private node of candidate */

				/* scan the private nodes */
 for(oldan = GetFnNextPrivateAd(fn_id,(AN_Id) NULL);
		oldan != NULL;
		oldan = GetFnNextPrivateAd(fn_id,oldan))
				/* only consider NAQs */
				/* (ENAQs and SENAQs use same nodes) */
	{if(!IsAdNAQ(oldan))
		continue;
	 switch(an_mode = GetAdMode(oldan))
		{
		 case Disp:
			regid = GetAdRegA(oldan);
			if(regid == CAP)
				{expr = GetAdExpression(Tbyte,oldan);
				 newan = GetAdDispInc(Tbyte,expr,CFP,
					(long int) (offset));
						/* We used Tbyte instead of */
						/* figuring out the correct */
						/* type in the above two inst */
						/* ructions because it was */
						/* too much trouble to get */
						/* the correct type, and it */
						/* is sufficient to be	*/
						/* consistant. */
				}
		 	else if(regid == CFP)
				newan = GetAdAddToKey(Tbyte,oldan,
					(long int) (offset
					+ nargs * TySize(Tword)));
						/* Tbyte for same reason as */
						/* above.	*/
			else fatal("ilcopynaqs: illegal register (%d).\n",
					an_mode);
			endcase;
		 case Immediate:
		 case CPUReg:
		 case MAUReg:
			continue;	/* no NAQ info */
		 case Absolute:
		 case AbsDef:
		 case StatCont:
		 case DispDef:
		 case IndexRegDisp:
		 case IndexRegScaling:
		 case PreDecr:
		 case PreIncr:
		 case PostDecr:
		 case PostIncr:
			fatal("ilcopynaqs: not a NAQ (%d).\n",an_mode);
			/*NOTREACHED*/
		 default:
			fatal("ilcopynaqs: unknown mode (%d).\n",an_mode);
			endcase;
		}
					/* copy the info */
	 PutAdGnaqType(newan,GetAdGnaqType(oldan));
	 PutAdFP(newan,
		IsAdFP(oldan) ? YES : (IsAdNotFP(oldan) ? NO : MAYBE) );
	 PutAdGNAQSize(newan,GetAdGNAQSize(oldan));
					/* copy alias info */
	 if(IsFnAlias(fn_id,oldan))
		PutFnAlias(FuncId,newan);
	}
}
		
	STATIC void
ilfloat(pif,pil,nargs,offset)	/* remove conversions for float */
				/* arguments and return values */
TN_Id pif;			/* node preceding expanded routine */
TN_Id pil;			/* node following expanded routine */
int nargs; 			/* number of words of arguments */
unsigned long int offset;	/* frame offset of function */

{extern unsigned int Max_Ops;	/* maximum operands in an instruction */
 extern int TySize();		/* returns size in bytes of given type */
 register AN_Id an_id;		/* general address node id */
 boolean flag;			/* general flag */
 unsigned int i;		/* operand counter */
 unsigned int movoff;		/* offset of move instruction */
 extern unsigned int ndisc;	/* Number of instructions discarded.	*/
 OperandType optyped;		/* Operand type of destination.	*/
 register TN_Id pn;		/* general text node id */
 extern void prafter();		/* debug before window print; in debug.c */
 extern void prbefore();	/* debug before window print; in debug.c */
 register TN_Id qn;		/* general text node id */
 TN_Id qqn;			/* general text node id */

 for(pn = pif; !IsTxBr(pn) && !IsTxLabel(pn) && pn; pn = GetTxPrevNode(pn))
	{if(IsTxAux(pn))
		continue;
	 an_id = GetTxOperandAd(pn,DESTINATION);
	 if(!IsAdNumber(an_id))
		continue;
	 optyped = GetTxOperandType(pn,DESTINATION);
	 movoff = GetAdNumber(optyped,an_id);
	 			/* look for mmovdd O1,n(%fp) */
	 if(GetTxOpCodeX(pn) == G_MMOV
			&& (GetTxOperandType(pn,2) == Tdouble)
			&& (optyped == Tdouble)
			&& (IsAdDisp(an_id) && GetAdRegA(an_id) == CFP)
			&& (offset <= movoff)
			&& (movoff < (offset + TySize(Tword) * nargs))
			&& IsAdNAQ(an_id))
				 /* look for optional mmovd_ n(%fp),__ */
		{for(qn = GetTxNextNode(pif);
				(GetTxOpCodeX(qn) == G_MMOV)
					|| (GetTxOpCodeX(qn) == G_MOV);
				qn = GetTxNextNode(qn))
			{if(an_id == GetTxOperandAd(qn,2))
			     {if(optyped == GetTxOperandType(qn,2)
					&& legalgen( G_MMOV,
						GetTxOperandType(pn,0),
						GetTxOperandMode(pn,0),
						GetTxOperandType(pn,1),
						GetTxOperandMode(pn,1),
						GetTxOperandType(pn,2),
						GetTxOperandMode(pn,2),
						GetTxOperandType(qn,
							DESTINATION),
						GetTxOperandMode(qn,
							DESTINATION)))
				 		/*check for other use */
				{flag = FALSE;
				 for(qqn = GetTxNextNode(pif);
						qqn != pil;
						qqn = GetTxNextNode(qqn))
					{if(IsTxAux(qqn)) continue;
					 if(qqn == qn) continue;
					 for(i = 0;i<Max_Ops;i++)
						{if(GetTxOperandType(qqn,i)
								== Tnone)
							continue;
						 if(GetTxOperandAd(qqn,i)
						    == GetTxOperandAd(qn,2))
							flag = TRUE;
						}
					 if(flag) break;
					}
				 if(flag) break;
						/* also quit if anything used by
						 * dest of G_MMOV n(%fp),- is set
						 * by any instruction between the
						 * two G_MMOV's.
						 */
				 for(qqn = GetTxNextNode(pn);
						qqn != qn;
						qqn = GetTxNextNode(qqn))
					{if(IsTxAux(qqn) || !IsTxGenOpc(qqn)) continue;
					 if(IsAdUses(GetTxOperandAd(qn,DESTINATION),
						GetTxOperandAd(qqn,3)))
						{
						 flag = TRUE;
						 break;
						}
					}
				 if(flag) break;
			    			/* okay to merge moves */
				 if(DBdebug(0,(XIL|XIL_FP)))
					prbefore(GetTxPrevNode(pn),
						GetTxNextNode(qn),0,
				    		"in-line expan fp arg conv");

				 PutTxOperandAd(pn,DESTINATION,
					GetTxOperandAd(qn,DESTINATION));
				 PutTxOperandType(pn,DESTINATION,
					GetTxOperandType(qn,DESTINATION));
				 qqn = GetTxNextNode(qn);
				 DelTxNode(qn);
				 ndisc += 1;	/* Update number discarded. */

				 if(DBdebug(0,(XIL|XIL_FP)))
					prafter(GetTxPrevNode(pn),qqn,0);
				}
			      break;
			     }
			}
		}
	}

				/* remove jump and label if not used */
 pn = GetTxPrevNode(pil);			/* Two pieces because if */
 pn = GetTxPrevNode(pn);			/* MACRO, expression too */
						/* is too complex.	*/
 if(IsTxLabel(GetTxPrevNode(pil)) && IsTxBr(pn) && 
		GetTxOperandAd(GetTxPrevNode(pil),0) == GetP(pn))
	{for(qn = pif; qn != pn; qn = GetTxNextNode(qn))
		{if(IsTxBr(qn) && GetP(qn) 
				== GetTxOperandAd(GetTxPrevNode(pil),0))
			return;
		}
	 DelTxNode(GetTxPrevNode(pil));
	 DelTxNode(GetTxPrevNode(pil));
	 ndisc += 2;				/* Update number discarded. */
	}

					/* look for float return */
 pn = GetTxPrevNode(pil);
 if( pn == pif )
	return;				/* no body left in function */
 qn = GetTxNextNode(pil);

					/* without push */
 an_id = GetTxOperandAd(pil,1);
 if(GetTxOpCodeX(pn) == G_MMOV 
		&& (GetTxOperandType(pn,DESTINATION)  == Tdouble)
		&& GetTxOpCodeX(pil) == G_MMOV
		&& GetTxOperandType(pil,2) == Tdouble
		&& GetTxOperandAd(pn,DESTINATION) == GetTxOperandAd(pil,2)
		&& legalgen( G_MMOV, 
			GetTxOperandType(pil,0), 
			GetTxOperandMode(pil,0),
			GetTxOperandType(pil,1), 
			GetTxOperandMode(pil,1),
			GetTxOperandType(pn,2), 
			GetTxOperandMode(pn,2),
			GetTxOperandType(pil,DESTINATION), 
			GetTxOperandMode(pil,DESTINATION)))
				/* merge moves */
	{if(DBdebug(0,(XIL|XIL_FP)))
		prbefore(GetTxPrevNode(pn),GetTxNextNode(pil),
			0,"in-line expansion fp return conversion");

	 PutTxOperandAd(pil,2,GetTxOperandAd(pn,2));
	 PutTxOperandType(pil,2,GetTxOperandType(pn,2));
	 qn = GetTxPrevNode(pn);
	 DelTxNode(pn);
	 ndisc += 1;				/* Update number discarded. */

	 if(DBdebug(0,(XIL|XIL_FP))) prafter(qn,GetTxNextNode(pil),0);
					/* look for optional move */
	 pn = GetTxPrevNode(pil);
	 if(pn == pif)
		return;			/* no body left in function */
	 if(GetTxOpCodeX(pn) == G_MMOV
			&& (GetTxOperandType(pn,DESTINATION) == Tdouble)
			&& GetTxOpCodeX(pil) == G_MMOV
			&& GetTxOperandType(pil,2) == Tdouble
			&& (GetTxOperandAd(pn,DESTINATION) ==
					GetTxOperandAd(pil,2))
			&& legalgen(G_MMOV, 
				GetTxOperandType(pil,0),
				GetTxOperandMode(pil,0),
				GetTxOperandType(pil,1),
				GetTxOperandMode(pil,1),
				GetTxOperandType(pn,2),
				GetTxOperandMode(pn,2),
				GetTxOperandType(pil,DESTINATION),
				GetTxOperandMode(pil,DESTINATION)))
					/* merge moves */
		{if(DBdebug(0,(XIL|XIL_FP)))
			prbefore(GetTxPrevNode(pn),GetTxNextNode(pil),0, 
				"in-line expansion fp return conversion");

		 PutTxOperandAd(pil,2,GetTxOperandAd(pn,2));
		 PutTxOperandType(pil,2,GetTxOperandType(pn,2));
		 qqn = GetTxPrevNode(pn);
		 DelTxNode(pn);
		 ndisc += 1;			/* Update number discarded. */

		 if(DBdebug(0,(XIL|XIL_FP))) prafter(qqn,GetTxNextNode(pil),0);
		}
	}
			/* with push */
 else if(GetTxOpCodeX(pn) == G_MMOV
		&& (GetTxOperandType(pn,DESTINATION) == Tdouble)
		&& GetTxOpCodeX(pil) == G_ADD3 
		&& IsAdImmediate(an_id) && IsAdNumber(an_id)
		&& GetTxOperandAd(pil,2) == GetAdCPUReg(CSP)
		&& (GetTxOperandAd(pil,DESTINATION) == GetAdCPUReg(CSP))
		&& GetTxOpCodeX(qn) == G_MMOV
		&& GetTxOperandType(qn,2) == Tdouble 
		&& (GetTxOperandAd(pn,DESTINATION) == GetTxOperandAd(qn,2))
		&& legalgen(G_MMOV, 
			GetTxOperandType(qn,0), 
			GetTxOperandMode(qn,0),
			GetTxOperandType(qn,1), 
			GetTxOperandMode(qn,1),
			GetTxOperandType(pn,2), 
			GetTxOperandMode(pn,2),
			GetTxOperandType(qn,DESTINATION), 
			GetTxOperandMode(qn,DESTINATION)))
				/* merge moves */
	{if(DBdebug(0,(XIL|XIL_FP)))
		prbefore(GetTxPrevNode(pn),GetTxNextNode(qn),0,
			"in-line expansion fp return conversion");

	 PutTxOperandAd(qn,2,GetTxOperandAd(pn,2));
	 PutTxOperandType(qn,2,GetTxOperandType(pn,2));
	 qqn = GetTxPrevNode(pn);
	 DelTxNode(pn);
	 ndisc += 1;				/* Update number discarded. */

	 if(DBdebug(0,(XIL|XIL_FP))) prafter(qqn,GetTxNextNode(qn),0);
					/* look for optional move */
	 pn = GetTxPrevNode(pil);
	 if(pn == pif)
		return;			/* no body left in function */
	 if(GetTxOpCodeX(pn) == G_MMOV 
			&& (GetTxOperandType(pn,DESTINATION) == Tdouble)
			&& GetTxOpCodeX(qn) == G_MMOV 
			&& GetTxOperandType(qn,2) == Tdouble
			&& (GetTxOperandAd(pn,DESTINATION ) ==
					GetTxOperandAd(qn,2))
			&& legalgen(G_MMOV, 
				GetTxOperandType(qn,0), 
				GetTxOperandMode(qn,0),
				GetTxOperandType(qn,1), 
				GetTxOperandMode(qn,1),
				GetTxOperandType(pn,2), 
				GetTxOperandMode(pn,2),
				GetTxOperandType(qn,DESTINATION), 
				GetTxOperandMode(qn,DESTINATION)))
					/* merge moves */
		{if(DBdebug(0,(XIL|XIL_FP)))
			prbefore(GetTxPrevNode(pn),GetTxNextNode(qn),0, 
				"in-line expansion fp return conversion");

		 PutTxOperandAd(qn,2,GetTxOperandAd(pn,2));
		 PutTxOperandType(qn,2,GetTxOperandType(pn,2));
		 qqn = GetTxPrevNode(pn);
		 DelTxNode(pn);
		 ndisc += 1;			/* Update number discarded. */

		 if(DBdebug(0,(XIL|XIL_FP))) prafter(qqn,GetTxNextNode(qn),0);
		}
	}
}

	void
ilsummary()			/* print summary of in-line expansion */

{extern boolean zflag;		/* flag for in-line debug; in local.c */
 char *expr;			/* Expression part of address.	*/
 FN_Id fn_id;			/* function id */
 extern int intpc;		/* percent limit on in-line; in GlobalDefs.c */
 int size, sumsize;		/* size summation variables */
 unsigned long totalni = 0;	/* total number ofinstructions in file */
	
 if(zflag)				 /* analytic printout */
	{sumsize = 0;
	 fprintf(stderr, "in-line expansion limit = %d\n", intpc);
					/* compute total instructions */
	 for(fn_id = GetFnNextNode((FN_Id) NULL); 
			fn_id;
			fn_id = GetFnNextNode(fn_id))
		if(!IsFnLibrary(fn_id)) 
			totalni += GetFnInstructions(fn_id);
					/* print for each function */
	 for(fn_id = GetFnNextNode((FN_Id) NULL);
			fn_id;
			fn_id = GetFnNextNode(fn_id))
		{if(totalni) size = GetFnExpansions(fn_id) * 
			(GetFnInstructions(fn_id) - 2 ) * 100 / totalni;
		 else
			size = 0;
		 sumsize += size;
		 expr = GetAdExpression(Tbyte,GetFnName(fn_id));
		 fprintf(stderr,
			"%s expansions=%d inst=%d sz=%d%% t_sz=%d%% \n",
			expr,
			(int)GetFnExpansions(fn_id),(int)GetFnInstructions(fn_id),
			size,sumsize);
		}
	}
}
