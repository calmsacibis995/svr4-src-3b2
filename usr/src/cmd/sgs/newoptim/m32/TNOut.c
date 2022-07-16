/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)newoptim:m32/TNOut.c	1.8"

/************************************************************************/
/*				TNOut.c					*/
/*									*/
/*		This file is meant to contain all the machine-dependant	*/
/*	code needed to print instruction nodes.				*/
/*									*/
/************************************************************************/

#include	<ctype.h>
#include	<stdio.h>
#include	"defs.h"
#include	"LoopTypes.h"
#include	"OpTabTypes.h"
#include	"OpTabDefs.h"
#include	"OperndType.h"
#include	"RegId.h"
#include	"ANodeTypes.h"
#include	"ANodeDefs.h"
#include	"olddefs.h"
#include	"RoundModes.h"
#include	"TNodeTypes.h"
#include	"TNodeDefs.h"
#include	"optab.h"
#include	"optim.h"
#include	"optutil.h"

	/* Private function declarations. */
STATIC boolean _AnyType();	/* TRUE if register dst that can be any type. */
STATIC int _UseExplicit();	/* Puts in explicit expand bytes; in this file.*/

	void
fprinst(outfp,cmt_flag,tn_id)	/* Print instruction with a newline. */
FILE *outfp;			/* Stream on which to print. */
int cmt_flag;			/* If -2, prints as a comment. */
TN_Id tn_id;			/* TN_Id of text node to print. */

{extern void fatal();		/* Handles fatal errors; in common. */
 extern int fprinstx();		/* Prints instruction without newline. */

 if(tn_id == NULL)		/* See if we've got a node id.*/
	fatal("fprinst: NULL tn_id.\n");	/* We don't have one.	*/
 (void) fprinstx(outfp,cmt_flag,tn_id);		/*Print instr without newline.*/
 putc(NEWLINE,outfp);				/* Append a newline. */
}
	int
fprinstx(outfp,cmt_flag,tn_id)	/* Print an instruction without newline. */
FILE *outfp;			/* Output stream on which to print.*/
int cmt_flag;			/* If -2, prints as a comment. */
TN_Id tn_id;			/* TN_Id of text node to print. */
	
{STATIC boolean _AnyType();	/* TRUE if register dst that can be any type. */
 STATIC int _UseExplicit();	/*Puts in explicit expand bytes; in this file.*/
 extern char *GetExtTypeId();	/* Gets external form of type id. */
 extern char *GetExtLoopType();	/* Gets external form of loop type. */
 extern char *GetSExtTypeId();	/* Gets short external form of type id. */
 extern unsigned int Max_Ops;	/* Maximum operands in an instruction.	*/
 unsigned OpCodeX;		/* Operation code index for this node. */
 unsigned long int UniqueId;	/* Node's unique identifier. */
 AN_Id an_id;
 extern void fatal();		/* Handles fatal errors; in common. */
 extern void print_line_info(); /* In DebugInfo.c */
 register int j, n;
 int ntot = 0;			/* Output count */
 unsigned int operand;		/* Operand counter. */
 extern struct opent optab[];	/* Operation code table; in optab.c */
 char *p;
 extern unsigned int praddr();	/* Prints an address; in mach. dep. */
 OperandType dflttype[4];
 OperandType td;
 OperandType tp;
 OperandType xtp;

 OpCodeX = GetTxOpCodeX(tn_id);			/* Get node's op-code index.*/
 UniqueId = GetTxUniqueId(tn_id); /* See if this node has a source linenumber */

 if(UniqueId != IDVAL) {
	if(cmt_flag==-2) print_line_info("#",UniqueId); /* comment out entry */
	else print_line_info("",UniqueId);
	 /*old code: fprintf(outfp,"\t.ln\t%d\n",UniqueId); */	/*Line number.*/
 }

		/* Write out instruction. */

 if(cmt_flag == -2)				/* If comment, */
	{putc(ComChar,outfp);		/* put one out. */
	ntot++;				/* accumulate output count. */
	}

 if(OpCodeX > GUPPER)				/* If illegal op-code,	*/
	OpCodeX = BLOWER;			/* make it "UNDEFINED",	*/
						/* for debugging.	*/

 switch(OpCodeX)
	{case BLOWER:				/* UNDEFINED */
	 case TAIL:				/* and TAIL */
	 case FILTER:
		ntot += fprintf(outfp,"\t%s\t",optab[OpCodeX].oname);
		endcase;
	 case ASMS:
		ntot += praddr(GetTxOperandAd(tn_id,0),Tnone,outfp);
		endcase;
	 case MISC:
		ntot += 7 + fprintf(outfp,"\t%s",optab[OpCodeX].oname);
		endcase;
	 case HLABEL:
	 case LABEL:
		ntot += fprintf(outfp,"%s:",
			GetAdExpression(Tbyte,GetTxOperandAd(tn_id,0)));
		endcase;
	 case LOOP:
		ntot += 1 + fprintf(outfp,"%c%s\t%s depth=%u serial=%u B_L_OK=%s",
			ComChar,
			optab[OpCodeX].oname, 
			GetExtLoopType(GetTxLoopType(tn_id)),
			GetTxLoopDepth(tn_id),
			GetTxLoopSerial(tn_id),
			GetTxLoopFlag(tn_id) ? "YES" : "NO");
		endcase;
	 case PS_ALIGN:
		ntot += 8 + fprintf(outfp,"\t.align\t%s",
			GetAdExpression(Tnone,GetTxOperandAd(tn_id,0)));
		endcase;
	 case TYPE:
		ntot += 3 + fprintf(outfp,
			"%c%s\t",ComChar,optab[OpCodeX].oname);
		for(p = GetExtTypeId(GetTxOperandType(tn_id,0));
				*p != EOS; p++)
			{putc(_toupper(*p), outfp);
			 ntot++;
			}
		endcase;
	 default:				/* Print opcode. */
		n = fprintf(outfp,"\t%s",optab[OpCodeX].oname);

		if(IsOpGeneric(OpCodeX))
			{for( ; n < 12; n++)
				putc(SPACE,outfp);
			 ntot = 19;
			}
		else
			{putc(TAB,outfp);
			 ntot = 16;
			}

					/* Set default operand types. */
		for(operand = 0; operand < Max_Ops; operand++)
			dflttype[operand] = (OperandType)
				optab[GetTxOpCodeX(tn_id)].otype[operand];

						/* Print operands. */
						/* Get operand parameters. */
		for(operand = 0; operand < Max_Ops; operand++)
			{tp = GetTxOperandType(tn_id,operand);
			 an_id = GetTxOperandAd(tn_id,operand);
			 if(an_id == NULL)	/* If trouble,	*/
				fatal("fprinstx: trouble.\n");

						/* Generic opcodes. */
			 if(IsOpGeneric(OpCodeX)) 
				{	/* print type and address */
				 fprintf(outfp,
					"%-2s ",GetSExtTypeId(tp));
				 if(IsAdCPUReg(an_id))
					{if(tp == Tsingle)
						xtp = T1word;
					 else if(tp == Tdouble)
						xtp = T2word;
					 else if(tp == Tdblext)
						xtp = T3word;
					}
				 else
					xtp = tp;
				 n = praddr(an_id,xtp,outfp);
				 for( ; n < 8; n++)
					putc(SPACE,outfp);
				 putc(SPACE,outfp);
				 ntot += 12;
				 continue;
				}

			 else 			/* Non-generic opcodes. */
				{td = dflttype[operand];
				 if(tp == Tnone)
					break;
				 if(operand != 0)
					{putc(COMMA,outfp);
					 ntot++;
					}
						/* Floating point mismatch. */
				 if((tp != td) && IsFP(tp) && (tp != Tdblext))
					goto error;
						/* Handle integer mismatch. */
				 if((tp != td) &&	/* If types different */
						!IsFP(tp) &&

							/* and not immediate */
						! (IsAdImmediate(an_id) &&
							/* and both signed */
							/* or both unsigned */
						(IsSigned(tp) == IsSigned(td)))
							/* Note: The sign */
							/* constraint is */
							/* required by the */
							/* hdwe.  The size */
							/* of an immediate */
							/* is always word, */
							/* but its */
							/* "signedness" comes */
							/* from instr and */
							/* operands. */

							/* and not reg dst */
							/* that can be any */
							/* type */
#ifdef IMPEXPAND
						&& !_AnyType(tn_id,operand)
#endif /* IMPEXPAND */
						)
					{if((!IsOpCpu(OpCodeX) ||
							!IsExpand(tp)))
						goto error;

					/* Use explicit expand bytes to avoid 
					assembler problem with {byte}. */
					 ntot += _UseExplicit(tp,outfp);

					 for(j = operand + 1; j < Max_Ops; j++) 
						{if(dflttype[j] == Tnone) 
							continue;
						 dflttype[j] = tp;
						}
					}
				}

						/* Print address. */
			 ntot += praddr(an_id,tp,outfp);
			}
		endcase;
	}
 return( ntot );

error:
	{extern char *extaddr();

	 fatal("fprinstx: incompatible operand types (%hu) (%s %s) (%s)%c.\n",
		GetTxOpCodeX(tn_id),
			GetExtTypeId(tp),extaddr(an_id,tp),
			GetExtTypeId(td),(IsExpand(tp)) ? 'T' : 'F');
	 /*NOTREACHED*/
	}
}

#ifdef IMPEXPAND
	STATIC boolean
_AnyType(tn_id,i)		/* TRUE if register destination that can */
				/* be any type */
TN_Id tn_id;			/* node being printed */
unsigned int i;			/* operand being printed */

{extern boolean islivecc();	/* TRUE if condition codes live after node */
 short o;			/* Associated opcode. */

				/* must be a register */
 if(!IsAdCPUReg(GetTxOperandAd(tn_id,i))) 
	return(FALSE);
				/* condition codes must be dead */
 if(islivecc(tn_id)) 
	return(FALSE);
				/* must be a pure destination */
 o = optab[GetTxOpCodeX(tn_id)].oopcode; /* look at associated generic opcode */
 switch(i)
	{case 0: 
		switch(o)
			{case G_POP:
				return(TRUE);
			}
		break;
	 case 1: 
		switch(o)
			{case G_MOVA:
			 case G_MOV:
			 case G_MNEG:
			 case G_MCOM:
				return(TRUE);
			}
		break;
	 case 2:
		switch(o)
			{case G_ADD3:
			 case G_ALS3:
			 case G_AND3:
			 case G_ARS3:
			 case G_LLS3:
			 case G_LRS3:
			 case G_MOD3:
			 case G_OR3:
			 case G_SUB3:
			 case G_XOR3:
				return(TRUE);
			 case G_DIV3:
			 case G_MUL3:
					/* assembler warns when the destination of
					 * these instr is unsigned and the sources
					 * have mixed signs. so return FALSE to
					 * force an explicit expand byte.
					 */
				return FALSE;
			}
		break;
	}
 return(FALSE);
}
#endif /* IMPEXPAND */
	STATIC int
_UseExplicit(tp,outfp)		/* Insert explicit expand byte. */
OperandType tp;			/* Operand Type. */
FILE *outfp;			/* Stream on which to write. */

{extern char *GetExtTypeId();	/* Get external type id; in Mach. Dep. */
 /*extern int fprintf();	** Prints to stream; in C(3) library. */
 int nout;			/* number of output chars. */

 switch(tp)
	{case Tbyte:
		nout = fprintf(outfp,"{%s}",GetExtTypeId(Tubyte));
		endcase;
	case Thalf:
		nout = fprintf(outfp,"{%s}",GetExtTypeId(TSHALF));
		endcase;
	case Tword:
		nout = fprintf(outfp,"{%s}",GetExtTypeId(TSWORD));
		endcase;
	default:
		nout = fprintf(outfp,"{%s}",GetExtTypeId(tp));
		endcase;
	}
 return(nout);
}
