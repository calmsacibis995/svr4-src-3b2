/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)newoptim:m32/IsLegal.c	1.6"

/************************************************************************/
/*				IsLegal.c				*/
/*									*/
/*		IsLegalAd returns TRUE if the address that would	*/
/*	be formed by its arguments is legal for the target machine.	*/
/*	Otherwise, it returns FALSE.					*/
/*									*/
/*		IsLegalInst returns TRUE if the instruction that would	*/
/*	be formed by its arguments is legal for the target machine.	*/
/*	Otherwise, it returns FALSE.					*/
/*									*/
/*									*/
/************************************************************************/

#include	"defs.h"
#include	"ANodeTypes.h"
#include	"olddefs.h"
#include	"OpTabTypes.h"
#include	"OpTabDefs.h"
#include	"RegId.h"
#include	"RegIdDefs.h"
#include	"OperndType.h"
#include	"Target.h"
#include	"optab.h"

	/* Private functions */
STATIC boolean ValCPUReg();
STATIC boolean ValMAUReg();
STATIC boolean ValStatCont();

	boolean					/* Test if address is legal. */
IsLegalAd(mode,rega,regb,constant,string)
AN_Mode mode;			/* Mode of operand.	*/
RegId rega;			/* First register of an operand.	*/
RegId regb;			/* Second register of an operand.	*/
long int constant;		/* Constant of operand.	*/
char *string;			/* String of operand. */

{extern void fatal();		/* Handles fatal errors; in common.	*/
 extern m32_target_cpu cpu_chip;	/* Target CPU chip.	*/

 switch(mode)					/* Test depends on mode. */
	{case Absolute:
	 case AbsDef:
	 case Immediate:
	 case Raw:
		return(TRUE);			/* Looks OK. */
		/*NOTREACHED*/
		endcase;
	 case CPUReg:
	 case Disp:
	 case DispDef:
		if(ValCPUReg(rega))		/* Make sure legal for us. */
			return(TRUE);
		endcase;
	 case StatCont:
		if(ValStatCont(rega))		/* Make sure legal for us. */	
			return(TRUE);
		endcase;
	 case IndexRegDisp:
		switch(cpu_chip)
			{case we32001:		/* Doesn't work on	*/
			 case we32100:		/* some chips.	*/
				endcase;
			 case we32200:		/* Does work on these.	*/
						/* Both registers must	*/
						/* be valid CPU registers. */
				if(!ValCPUReg(rega))
					return(FALSE);
				if(!ValCPUReg(regb))
					return(FALSE);
				if(GetRegNo(rega) <= 8)	/* They must be */
					{if((16 <= GetRegNo(regb)) &&
							(GetRegNo(regb) <= 31))
						return(TRUE);	/* from */
					 else			/* different */
						return(FALSE);	/* sets. */
					}
				if((16 <= GetRegNo(rega)) &&
						(GetRegNo(rega) <= 31))
					{if(GetRegNo(regb) <= 8)
						return(TRUE);
					 else
						return(FALSE);
					}
				endcase;
			 default:
				fatal("IsLegal: unsupported cpu chip.\n");
				endcase;
			}
		endcase;
	 case IndexRegScaling:
		if(cpu_chip != we32200)		/* This won't work on 32100. */
			return(FALSE);
		if(!ValCPUReg(rega))		/* Both registers must be */
			return(FALSE);		/* valid CPU registers. */
		if(!ValCPUReg(regb))
			return(FALSE);
		if(cpu_chip == we32200)		/* No displacement for 32200.*/
			{if(*string != EOS)
				return(FALSE);	/* There was an expression. */
			 if(constant != 0)
				return(FALSE);		/* Error otherwise. */
			if(GetRegNo(rega) <= 8)	/* Scaled one must be from */
				{if((16 <= GetRegNo(regb)) &&	/* lower set, */
						(GetRegNo(regb) <= 31))
					return(TRUE);	/* and other one */
				 else			/* must be from */
					return(FALSE);	/* upper set. */
				}
			} /* END OF if(cpu_chip == we32200) */
		endcase;
	 case MAUReg:
		if(ValMAUReg(rega))		/* Make sure legal for us. */
			return(TRUE);
		endcase;
	 case PostDecr:
		if(cpu_chip != we32200)		/* Won't work on 32100. */
			return(FALSE);
		if(cpu_chip == we32200)		/* No displacement for 32200.*/
			if(*string != EOS)
				return(FALSE);	/* There was an expression. */
		if(ValCPUReg(rega))		/* Make sure legal for us. */
			return(TRUE);
		endcase;
	 case PostIncr:
		if(cpu_chip != we32200)		/* Won't work on 32100. */
			return(FALSE);
		if(cpu_chip == we32200)		/* No displacement for 32200.*/
			if(*string != EOS)
				return(FALSE);	/* There was an expression. */
		if(ValCPUReg(rega))		/* Make sure legal for us. */
			return(TRUE);
		endcase;
	 case PreDecr:
		if(cpu_chip != we32200)		/* Won't work on 32100. */
			return(FALSE);
		if(cpu_chip == we32200)		/* No displacement for 32200.*/
			if(*string != EOS)
				return(FALSE);	/* There was an expression. */
		if(ValCPUReg(rega))		/* Make sure legal for us. */
			return(TRUE);
		endcase;
	 case PreIncr:
		if(cpu_chip != we32200)		/* Won't work on 32100. */
			return(FALSE);
		if(cpu_chip == we32200)		/* No displacement for 32200.*/
			if(*string != EOS)
				return(FALSE);	/* There was an expression. */
		if(ValCPUReg(rega))		/* Make sure legal for us. */
			return(TRUE);
		endcase;
	 default:
		fatal("IsLegalAd: unknown mode detected (0x%x).\n",mode);
		endcase;
	}
 return(FALSE);
}
	STATIC boolean
ValCPUReg(regid)		/*TRUE if valid CPU register for this machine.*/
RegId regid;			/* Identifier of register to be tested. */

{extern void fatal();		/* Handles fatal errors; in ???.c. */
 extern m32_target_cpu cpu_chip;	/* Target cpu chip.	*/

 switch(regid)					/* Test Register Id. */
	{case CREG0:
	 case CREG1:
	 case CREG2:
	 case CREG3:
	 case CREG4:
	 case CREG5:
	 case CREG6:
	 case CREG7:
	 case CREG8:
	 case CREG9:
	 case CREG10:
	 case CREG11:
	 case CREG12:
	 case CREG13:
	 case CREG14:
	 case CREG15:
	 case CTEMP:		/* Temps are not valid for any machine,	*/
				/*  but we fake it to quiet ANMake().	*/
				/* If this one seeps through, the 	*/
				/*  assembler will catch it.		*/
		return(TRUE);
		/*NOTREACHED*/
		endcase;
	 case CREG16:
	 case CREG17:
	 case CREG18:
	 case CREG19:
	 case CREG20:
	 case CREG21:
	 case CREG22:
	 case CREG23:
	 case CREG24:
	 case CREG25:
	 case CREG26:
	 case CREG27:
	 case CREG28:
	 case CREG29:
	 case CREG30:
	 case CREG31:
		if(cpu_chip == we32200)
			return(TRUE);
		endcase;
	 case MASR:
	 case MREG0:
	 case MREG1:
	 case MREG2:
	 case MREG3:
	 case MREG4:
	 case MREG5:
	 case MREG6:
	 case MREG7:
	 case CCODE_A:
	 case CCODE_C:
	 case CCODE_N:
	 case CCODE_V:
	 case CCODE_X:
	 case CCODE_Z:
	 case MFPR:
	 case REG_NONE:
		endcase;
	 default:
		fatal("ValCPUReg: unknown CPU register id (%d).\n",regid);
		endcase;
	}
 return(FALSE);
}
	STATIC boolean
ValMAUReg(regid)		/* TRUE if valid MAU register. */
RegId regid;			/* Identifier of register to be tested. */

{extern m32_target_math math_chip;	/* Math chip, if any, used by target. */
 extern void fatal();			/* Handles fatal errors; in common.	*/

 switch(regid)					/* Test Register Id. */
	{case CREG0:
	 case CREG1:
	 case CREG2:
	 case CREG3:
	 case CREG4:
	 case CREG5:
	 case CREG6:
	 case CREG7:
	 case CREG8:
	 case CREG9:
	 case CREG10:
	 case CREG11:
	 case CREG12:
	 case CREG13:
	 case CREG14:
	 case CREG15:
	 case CREG16:
	 case CREG17:
	 case CREG18:
	 case CREG19:
	 case CREG20:
	 case CREG21:
	 case CREG22:
	 case CREG23:
	 case CREG24:
	 case CREG25:
	 case CREG26:
	 case CREG27:
	 case CREG28:
	 case CREG29:
	 case CREG30:
	 case CREG31:
		endcase;		/* CPU reg is not MAU reg. */
	 case MASR:
	 case MREG0:
	 case MREG1:
	 case MREG2:
	 case MREG3:
		if(math_chip == we32106)
			return(TRUE);
		else if(math_chip == we32206)
			return(TRUE);
		endcase;
	 case MREG4:
	 case MREG5:
	 case MREG6:
	 case MREG7:
		if(math_chip == we32106)
			return(FALSE);
		else if(math_chip == we32206)
			return(TRUE);
		endcase;
	 case CCODE_A:
	 case CCODE_C:
	 case CCODE_N:
	 case CCODE_V:
	 case CCODE_X:
	 case CCODE_Z:
		endcase;		/* Cond. codes not CPU reg. *./
	 case MFPR:
		endcase;		/* MAU rounding not CPU reg. */
	 case REG_NONE:
		endcase;
	 default:
		fatal("ValMAUReg: unknown MAU register id (%d).\n",regid);
		endcase;
	}
 return(FALSE);
}
	STATIC boolean
ValStatCont(regid)		/* TRUE if valid status or control bit. */
RegId regid;			/* Identifier of register to be tested. */

{extern void fatal();		/* Handles fatal errors; in ???.c. */
 extern m32_target_cpu cpu_chip;	/* Target CPU chip.	*/

 switch(regid)					/* Test Register Id. */
	{case CREG0:
	 case CREG1:
	 case CREG2:
	 case CREG3:
	 case CREG4:
	 case CREG5:
	 case CREG6:
	 case CREG7:
	 case CREG8:
	 case CREG9:
	 case CREG10:
	 case CREG11:
	 case CREG12:
	 case CREG13:
	 case CREG14:
	 case CREG15:
	 case CREG16:
	 case CREG17:
	 case CREG18:
	 case CREG19:
	 case CREG20:
	 case CREG21:
	 case CREG22:
	 case CREG23:
	 case CREG24:
	 case CREG25:
	 case CREG26:
	 case CREG27:
	 case CREG28:
	 case CREG29:
	 case CREG30:
	 case CREG31:				/* CPU reg is not Cond. code. */
	 case MASR:
	 case MREG0:
	 case MREG1:
	 case MREG2:
	 case MREG3:
	 case MREG4:
	 case MREG5:
	 case MREG6:
	 case MREG7:				/* MAU reg. not cond. code. */
	 case REG_NONE:
		endcase;
	 case CCODE_C:
	 case CCODE_N:
	 case CCODE_V:
	 case CCODE_Z:
	 case CCODE_A:
	 case MFPR:
		return(TRUE);
		/*NOTREACHED*/
		endcase;
	 case CCODE_X:
		if(cpu_chip == we32200)
			return(TRUE);
		endcase;
	 default:
		fatal("ValStatCont: unknown Condition Code (%d).\n",regid);
		endcase;
	}
 return(FALSE);
}
	boolean
legalgen( op, t0, m0, t1, m1, t2, m2, t3, m3 )	/* check whether a generic
					corresponds to a real instruction */
unsigned short int op; 	/* Op-Code index of instruction to be tested.	*/
OperandType t0; AN_Mode m0; 
OperandType t1; AN_Mode m1;
OperandType t2; AN_Mode m2;
OperandType t3; AN_Mode m3;

{boolean ImpUseMau();
 extern unsigned int Max_Ops;	/* Maximum operands per instruction.	*/
 extern boolean chkmmov();
 register int i;
 extern boolean misinp;
 extern struct opent optab[];	/* The operation-code table.	*/
 OperandType t[4];
 OperandType ti, oti;
 AN_Mode m[4];
 struct opent *o = &optab[op];

 t[0] = t0; m[0] = m0;
 t[1] = t1; m[1] = m1;
 t[2] = t2; m[2] = m2;
 t[3] = t3; m[3] = m3;

 for(i = Max_Ops-1; i >= 0; i--)		/* Per operand checks.	*/
	{ti = t[i];
	 oti = (OperandType) o->otype[i];
	 switch(m[i])
		{case CPUReg: 
			if((op == G_MMOV) && !chkmmov(t2,m2,t3,m3))
				return( FALSE );
			if((op == G_MMOVFA) ||
					(op == G_MMOVTA) || 
					(op == MMOVFD) ||
					(op == MMOVTD))
				return(FALSE);
			if((op != G_MMOV) && (oti == Tfp))
				return(FALSE); 
			break;
		case MAUReg: 
			if(oti != Tfp)
				return( FALSE ); 
			break;
		}
	switch( oti )
		{case TBIN:   
			if(!IsExpand(ti))
				return (FALSE); 
			break;
		 case TFP:    
			if((op != G_MMOV) && !IsFP(ti))
				return(FALSE); 
			if((op != G_MMOV) && IsFP( t2 ) && ti != t2 )
				return( FALSE );
			if( op != G_MMOV && IsFP( t3 ) && ti != t3 )
				return( FALSE );
			break;
		 case TWORD:  
			if(ti != Tword)
				return(FALSE);
			break;
		 case TDECINT:
			if(ti != Tdecint)
				return(FALSE);
			break;
		}
	}

 switch(op)			/* per instruction checks */
	{case G_MOVA: 
		if((m2 == Immediate) || (m2 == CPUReg))
			return(FALSE);
		break;
	 case G_PUSHA: 
		if((m2 == Immediate) || (m2 == CPUReg))
			return(FALSE);
		break;
	 case G_SWAPI: 
		if((m3 == Immediate) || (m3 == CPUReg))
			return(FALSE);
		if((t3 != Tword) && (t3 != Thalf) && (t3 != Tbyte))
			return(FALSE);
		break;
	 case G_MMOV:
		if(!IsFP(t2) && !IsFP(t3))
			return(FALSE);
		if(!IsFP(t2) && (t2 != Tword) && (t2 != Tuword) && 
				(t2 != Tdecint))
			return(FALSE);
		if(!IsFP(t3) && (t3 != Tword) && (t3 != Tuword) && 
				(t3 != Tdecint))
			return(FALSE);
		/* prevent MIS code in FPE mode */
		if(!misinp && !chkmmov(t2,m2,t3,m3))
			return(FALSE);
		break;
	}

		/* check for implicit uses of MAU register.
		 * this applies for both 32106 and 32206.
		 */
 if(IsOpMIS(op) && ImpUseMau(op, t, m))
	return(FALSE);

 return( TRUE );
}
	boolean
ImpUseMau(op, t, m)	/* checks for implicit uses of MAU reg.
			 * this routine does not assume op is an internal
			 * generic, and can be called any point in the
			 * output process.
			 */
unsigned short int op; 	/* Op-Code index of instruction to be tested.	*/
OperandType *t; 
AN_Mode *m;
{
 extern unsigned int Max_Ops;	/* Maximum operands per instruction.	*/
 int MaxSrcTySize;
 extern int TySize();
 boolean allmem;
 boolean allsrcmem;
 int i, dest;
 unsigned dst;
 extern struct opent optab[];
 unsigned src;

	if( optab[op].oflags & IUMR0 )	/* never implicitly uses a MAU reg */
		return FALSE;
	if( optab[op].oflags & IUMR2 )	/* always implicitly uses a MAU reg */
		return TRUE;

	if( op == G_MMOV ){	/* special internal generic mmov cases */
		if( (IsFP(t[2]) && IsFP(t[3]) && t[2] == t[3]) ||
			(t[2] == Tsingle && t[3] == Tword) ||
			(t[3] == Tsingle && t[2] == Tword) ||
			(t[2] == Tdecint && t[3] == Tdblext) )
				return FALSE;
		if( (IsFP(t[2]) && t[3] == Tuword) ||
			(IsFP(t[3]) && t[2] == Tuword) )
				return TRUE;
	}
		/* collect operand info */
	dest = -1;
	MaxSrcTySize = 0;
	allmem = allsrcmem = TRUE;
	src = optab[op].osrcops;
	dst = optab[op].odstops;
	for( i = 0; i < Max_Ops ; ++i ){
		if( t[i] != Tnone ){
			if( m[i] == MAUReg ){
				/* assume that MAUReg is the only non-memory mode */
				allmem = FALSE;
				if(src & 01)
					allsrcmem = FALSE;
				}
			if( (src & 01) && TySize(t[i]) > MaxSrcTySize )
				MaxSrcTySize = TySize(t[i]);
			if( dst & 01 )
				dest = i;
			}
		src >>= 1;
		dst >>= 1;
	}

		/* if arithmetic and all source operands are memory */
	if( optab[op].oflags & ARITH ){
		if( allsrcmem )
			return TRUE;
		}
	else	/* if non arithmetic and all operands are memory */
		if( allmem )
			return TRUE;

		/* if dest operand is narrower than any source */
	if( dest >= 0 && TySize(t[dest]) < MaxSrcTySize )
		return TRUE;

	return FALSE;
}
