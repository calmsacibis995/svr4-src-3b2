/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)newoptim:m32/ANOut.c	1.9"

#include	<stdio.h>
#include	<malloc.h>
#include	<string.h>
#include	"defs.h"
#include	"RegId.h"
#include	"RegIdDefs.h"
#include	"ANodeTypes.h"
#include	"ANodeDefs.h"
#include	"OperndType.h"
#include	"olddefs.h"

#define	MAXREGSIZ	5	/* Max size for external register id. */


	unsigned int
praddr(an_id,type,stream)	/* Print address node. */
AN_Id an_id;			/* Id of address node to print. */
OperandType type;		/* Type of operand. */
FILE *stream;			/* Stream on which to write. */

{
 extern char *extaddr();	/*Converts address node to string; in this file.*/
 unsigned int n;		/* number of bytes output. */
 char *xaddr;

 xaddr = extaddr(an_id,type);
 fprintf(stream,"%s",xaddr);
 n = strlen(xaddr);
 return(n);
}
	char *
extaddr(an_id,type)		/* Gets external form of an address node */
/* Note: the following assumption is made about the use of this
   function: it is called only for the final printing of text nodes
   or to report fatal errors.  In particular, it is never used in
   the optimizations per se.  The reason for this assumption is
   that the code now modifies PIC style address nodes: nodes
   of type Disp or DispDef which are offsets off of %pc and for
   which the displacement is of the form <id>@PC, <id>@GOT, <id>@PLT
   are converted to PIC syntax.  The optimizer must not make any
   further use of this converted format. */

AN_Id an_id;			/* Id of address node to convert. */
OperandType type;		/* Type of address. */

{extern void ExtendBuf();	/* Extends string buffer. */
 static char *b0;		/* Pointer to output string. */
 static char *bn;		/* Pointer to end of string buffer. */
 extern enum CC_Mode ccmode;	/* cc -X?	*/
 int esize;			/* Size of expression part of address. */
 char *expression, *p;
 extern void fatal();		/* Handles fatal errors; in debug.c. */
 AN_Mode mode;			/* Mode of address. */
 RegId reg;
 char regA[MAXREGSIZ+1];	/* Place for external form of regA. */
 char regB[MAXREGSIZ+1];	/* Place for external form of regB. */
 boolean zero_extend;		/* Used for zero extending unsigned constants. */

 if(b0 == NULL)
	{b0 = Malloc(EBUFSIZE+1);
	 if(b0 == NULL)
		fatal("extaddr: out of buffer space\n");
	 bn = b0+EBUFSIZE;
	}
 mode = GetAdMode(an_id);			/* Get mode of address. */
 switch(mode)					/* Each one different. */
	{case Absolute:				/* Absolute. */
		expression = GetAdExpression(type,an_id);
		esize = strlen(expression);
		if(esize == 0)			/* If no expression,	*/
			{expression = "0";	/* fake it */
			 ++esize;
			}
		else if(PIC_flag && strchr(expression,AtChar))
		    fatal("extaddr(): illegal PIC expression %s\n",expression);
		if(b0+esize > bn)
			ExtendBuf(&b0,&bn,(unsigned)esize+1);
		(void) sprintf(b0,"%s",expression);
		endcase;
	 case AbsDef:				/* Absolute Deferred. */
		expression = GetAdExpression(type,an_id);
		esize = 1+strlen(expression);
		if(PIC_flag && strchr(expression,AtChar))
		    fatal("extaddr(): illegal PIC expression %s\n",expression);
		if(b0+esize > bn)
			ExtendBuf(&b0,&bn,(unsigned)esize+1);
		(void) sprintf(b0,"*%s",expression);
		endcase;
	 case CPUReg:				/* CPU Register. */
		if(IsAdTemp(an_id))
			(void) sprintf(b0,"%%t%d",GetAdTempIndex(an_id));
		else
			{GetExtRegId(GetAdRegA(an_id),Tnone,regA,sizeof(regA));
			 (void) sprintf(b0,"%s",regA);
			}
		endcase;
	 case StatCont:				/* Status or Control bit. */
		GetExtRegId(GetAdRegA(an_id),Tnone,regA,sizeof(regA));
		(void) sprintf(b0,"%s",regA);
		endcase;
	 case Disp:				/* Register-Displacement. */
		expression = GetAdExpression(type,an_id);
		esize = strlen(expression)+1+MAXREGSIZ+1;
		if(b0+esize > bn)
			ExtendBuf(&b0,&bn,(unsigned)esize+1);
		reg = GetAdRegA(an_id);
		if(reg == CTEMP)
			sprintf(regA,"%%t%d",GetAdTempIndex(an_id));
		else if( PIC_flag && (reg == CPC) && (p=strchr(expression,AtChar)) ) {
		    /* PIC code: modify X@PC(%pc)+junk to X+junk@PC
			X@GOT(%pc), and X@PLT(%pc) to X@GOT and X@PLT */
		    esize-=(MAXREGSIZ+1);
		    if((p[2] == 'C') && (p[3] != EOS)) { /* ..@PC+junk.. needs special treatment */
			*p = EOS;
		    	(void) sprintf(b0,"%s%s@PC",expression,p+3);
			*p = AtChar;
		    }
		    else
			(void) sprintf(b0,"%s",expression);
		    break;
		}
		else
			GetExtRegId(reg,Tnone,regA,sizeof(regA));
		(void) sprintf(b0,"%s(%s)",expression,regA);
		endcase;
	 case DispDef:				/* Displacement-Deferred. */
		expression = GetAdExpression(type,an_id);
		esize = 1+strlen(expression)+1+MAXREGSIZ+1;
		if(b0+esize > bn)
			ExtendBuf(&b0,&bn,(unsigned)esize+1);
		reg = GetAdRegA(an_id);
		if(reg == CTEMP)
			sprintf(regA,"%%t%d",GetAdTempIndex(an_id));
		else if(PIC_flag && (reg == CPC) && (p = strchr(expression,AtChar)) ) {
		    esize-=(MAXREGSIZ+1);
		    if((p[2] == 'C') && (p[3] != EOS)) { /* ..@PC+junk.. needs special treatment */
			*p = EOS;
		    	(void) sprintf(b0,"*%s%s@PC",expression,p+3);
			*p = AtChar;
		    }
		    else
			(void) sprintf(b0,"*%s",expression);
		    break;
		}
		else
			GetExtRegId(reg,Tnone,regA,sizeof(regA));
		(void) sprintf(b0,"*%s(%s)",expression,regA);
		endcase;
	 case Immediate:			/* Immediate. */
		expression = GetAdExpression(type,an_id);
		esize = 1+strlen(expression);
					/* Force the immediate to be a	*/
					/* a full word if an unsigned */
					/* data type would cause the cpu */
					/* to zero extend a byte or half word */
					/* immediate.	*/
					/* But do this only in transition mode*/
					/* since ANSI changes the semantics of*/
					/* such type conversions!	*/
		zero_extend = FALSE;
		if(ccmode == Transition
				&& IsAdNumber(an_id) 
				&& (expression[0] == '-') 
				&& !IsSigned(type))
			{zero_extend = TRUE;
			 esize += 3;
			}
		if(b0+esize > bn)
			ExtendBuf(&b0,&bn,(unsigned)esize+1);
		(void) sprintf(b0, zero_extend ? "&%s<W>" : "&%s",expression);
		endcase;
	 case IndexRegDisp:			/* Indexed Register with Disp.*/
		expression = GetAdExpression(type,an_id);
		esize = strlen(expression)+1+MAXREGSIZ+1+MAXREGSIZ+1;
		if(b0+esize > bn)
			ExtendBuf(&b0,&bn,(unsigned)esize+1);
		GetExtRegId(GetAdRegA(an_id),Tnone,regA,sizeof(regA));
		GetExtRegId(GetAdRegB(an_id),Tnone,regB,sizeof(regB));
		(void) sprintf(b0,"%s(%s,%s)",expression,regA,regB);
		endcase;
	 case IndexRegScaling:			/* Indexed Reg. with Scaling. */
		GetExtRegId(GetAdRegA(an_id),Tnone,regA,sizeof(regA));
		GetExtRegId(GetAdRegB(an_id),Tnone,regB,sizeof(regB));
		esize = MAXREGSIZ+1+MAXREGSIZ+1;
		if(b0+esize > bn)
			ExtendBuf(&b0,&bn,(unsigned)esize+1);
		(void) sprintf(b0,"%s[%s]",regA,regB);
		endcase;
	 case MAUReg:				/* MAU Register. */
		GetExtRegId(GetAdRegA(an_id),(type == Tdblext) ? Tdblext : Tany,
			regA,sizeof(regA));
		(void) sprintf(b0,"%s",regA);
		endcase;
	 case PostDecr:				/* Auto Post Decrement. */
		expression = GetAdExpression(type,an_id);
		esize = strlen(expression)+1+MAXREGSIZ+2;
		if(b0+esize > bn)
			ExtendBuf(&b0,&bn,(unsigned)esize+1);
		GetExtRegId(GetAdRegA(an_id),Tnone,regA,sizeof(regA));
		(void) sprintf(b0,"%s(%s)-",expression,regA);
		endcase;
	 case PostIncr:				/* Auto Post Increment. */
		expression = GetAdExpression(type,an_id);
		esize = strlen(expression)+1+MAXREGSIZ+2;
		if(b0+esize > bn)
			ExtendBuf(&b0,&bn,(unsigned)esize+1);
		GetExtRegId(GetAdRegA(an_id),Tnone,regA,sizeof(regA));
		(void) sprintf(b0,"%s(%s)+",expression,regA);
		endcase;
	 case PreDecr:				/* Auto Pre Decrement. */
		expression = GetAdExpression(type,an_id);
		esize = strlen(expression)+2+MAXREGSIZ+1;
		if(b0+esize > bn)
			ExtendBuf(&b0,&bn,(unsigned)esize+1);
		GetExtRegId(GetAdRegA(an_id),Tnone,regA,sizeof(regA));
		(void) sprintf(b0,"%s-(%s)",expression,regA);
		endcase;
	 case PreIncr:				/* Auto Pre Increment. */
		expression = GetAdExpression(type,an_id);
		esize = strlen(expression)+2+MAXREGSIZ+1;
		if(b0+esize > bn)
			ExtendBuf(&b0,&bn,(unsigned)esize+1);
		GetExtRegId(GetAdRegA(an_id),Tnone,regA,sizeof(regA));
		(void) sprintf(b0,"%s+(%s)",expression,regA);
		endcase;
	 case Raw:
		expression = GetAdExpression(type,an_id);
		esize = strlen(expression);
		if(b0+esize > bn)
			ExtendBuf(&b0,&bn,(unsigned)esize+1);
		(void) sprintf(b0,"%s",expression);
		endcase;
	 default:
		fatal("extaddr: invalid mode (0x%8.8x).\n",mode);
		endcase;
	}
 return(b0);
}
